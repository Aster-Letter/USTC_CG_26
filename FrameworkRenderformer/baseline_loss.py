from __future__ import annotations

from typing import Dict, Tuple

import torch
import torch.nn as nn
import torch.nn.functional as F


class SimpleRenderFormerLoss(nn.Module):
    """
    A lighter training loss for course use.

    The default balanced log-L1 keeps the cheap log-radiance objective, but
    upweights bright/visible regions. A plain unweighted log-L1 lets a black
    image score surprisingly well on this HDR dataset, which can pull the
    decoder into a near-black local optimum.
    """

    def __init__(
        self,
        loss_type: str = "balanced_log_l1",
        use_lpips: bool = False,
        lpips_weight: float = 0.05,
        device: str = "cpu",
    ):
        super().__init__()
        self.loss_type = loss_type
        self.use_lpips = use_lpips
        self.lpips_weight = lpips_weight
        self.device_name = device

        self.lpips_model = None
        if use_lpips:
            try:
                import lpips
            except ImportError as exc:
                raise ImportError("LPIPS is not installed. Disable --use_lpips or install lpips.") from exc

            self.lpips_model = lpips.LPIPS(net="vgg").to(device)
            self.lpips_model.eval()
            for parameter in self.lpips_model.parameters():
                parameter.requires_grad_(False)

    @staticmethod
    def _log_transform(image: torch.Tensor) -> torch.Tensor:
        return torch.log1p(torch.clamp(image, min=0.0))

    @staticmethod
    def _tone_map(image: torch.Tensor) -> torch.Tensor:
        return torch.pow(torch.clamp(image, 0.0, 1.0), 1.0 / 2.2)

    def _balanced_log_l1(self, prediction: torch.Tensor, target: torch.Tensor) -> torch.Tensor:
        pred_log = self._log_transform(prediction)
        target_log = self._log_transform(target)

        # Use log-luminance as a stable visibility proxy. The weight map is
        # normalized per image, so scenes with different exposure do not change
        # the global loss scale too much.
        target_log_luma = target_log.mean(dim=1, keepdim=True)
        mean_luma = target_log_luma.mean(dim=(1, 2, 3), keepdim=True).clamp_min(1e-4)
        weight = 1.0 + 3.0 * torch.clamp(target_log_luma / mean_luma, max=4.0)
        weight = weight.expand_as(pred_log)
        weighted_log_l1 = (torch.abs(pred_log - target_log) * weight).sum() / weight.sum().clamp_min(1e-6)

        # These small auxiliary terms align optimization with the report-facing
        # clamped/display metrics and make the all-black solution expensive.
        clamped_l1 = F.l1_loss(torch.clamp(prediction, 0.0, 1.0), torch.clamp(target, 0.0, 1.0))
        display_l1 = F.l1_loss(self._tone_map(prediction), self._tone_map(target))
        return weighted_log_l1 + 0.25 * clamped_l1 + 0.25 * display_l1

    def forward(self, prediction: torch.Tensor, target: torch.Tensor) -> Tuple[torch.Tensor, Dict[str, torch.Tensor]]:
        if self.loss_type == "mse":
            base_loss = F.mse_loss(prediction, target)
        elif self.loss_type == "l1":
            base_loss = F.l1_loss(prediction, target)
        elif self.loss_type == "log_l1":
            base_loss = torch.mean(torch.abs(self._log_transform(prediction) - self._log_transform(target)))
        elif self.loss_type == "balanced_log_l1":
            base_loss = self._balanced_log_l1(prediction, target)
        else:
            raise ValueError(f"Unsupported loss_type: {self.loss_type}")

        total_loss = base_loss
        lpips_loss = torch.zeros_like(base_loss)
        if self.lpips_model is not None:
            pred_lpips = self._tone_map(prediction) * 2.0 - 1.0
            target_lpips = self._tone_map(target) * 2.0 - 1.0
            lpips_loss = self.lpips_model(pred_lpips, target_lpips).mean()
            total_loss = total_loss + self.lpips_weight * lpips_loss

        metrics = {
            "total_loss": total_loss.detach(),
            "base_loss": base_loss.detach(),
            "lpips_loss": lpips_loss.detach(),
        }
        return total_loss, metrics
