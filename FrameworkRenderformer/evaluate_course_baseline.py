from __future__ import annotations

import argparse
import json
import sys
from contextlib import nullcontext
from pathlib import Path
from typing import Any, Dict, List

import torch
from torch.utils.data import DataLoader


SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from baseline_model import CourseRenderFormerWrapper, build_baseline_config
from train_course_baseline import (
    build_dataset,
    ensure_resolution_is_valid,
    move_to_device,
    pick_device,
    resolve_amp_mode,
    save_json,
    save_preview,
)
from baseline_data import renderformer_baseline_collate


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Evaluate a RenderFormer course checkpoint and compute PSNR.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument("--checkpoint", type=str, required=True)
    parser.add_argument("--out_dir", type=str, required=True)
    parser.add_argument("--dataset_format", choices=["pt", "h5"], default=None)
    parser.add_argument("--data_path", type=str, default=None)
    parser.add_argument("--max_items", type=int, default=None)
    parser.add_argument("--batch_size", type=int, default=1)
    parser.add_argument("--workers", type=int, default=0)
    parser.add_argument("--device", choices=["auto", "cuda", "cpu"], default="auto")
    parser.add_argument("--amp", choices=["auto", "bf16", "fp16", "none"], default="auto")
    parser.add_argument("--save_images", type=int, default=8)
    return parser.parse_args()


def load_checkpoint_args(checkpoint_path: Path) -> Dict[str, Any]:
    checkpoint = torch.load(checkpoint_path, map_location="cpu", weights_only=False)
    args = checkpoint.get("args")
    if not isinstance(args, dict):
        raise KeyError(f"Checkpoint does not contain serialized args: {checkpoint_path}")
    return checkpoint, args


def merge_eval_args(cli_args: argparse.Namespace, checkpoint_args: Dict[str, Any]) -> Dict[str, Any]:
    merged = dict(checkpoint_args)
    if cli_args.dataset_format is not None:
        merged["dataset_format"] = cli_args.dataset_format
    if cli_args.data_path is not None:
        merged["data_path"] = cli_args.data_path
    merged["max_items"] = cli_args.max_items if cli_args.max_items is not None else checkpoint_args.get("max_items")
    merged["batch_size"] = cli_args.batch_size
    merged["workers"] = cli_args.workers
    return merged


def append_jsonl(save_path: Path, content: Dict[str, Any]) -> None:
    with save_path.open("a", encoding="utf-8") as file:
        file.write(json.dumps(content, ensure_ascii=True) + "\n")


def to_display_space(image: torch.Tensor) -> torch.Tensor:
    image = torch.clamp(image, 0.0, 1.0)
    return torch.pow(image, 1.0 / 2.2)


def compute_psnr(prediction: torch.Tensor, target: torch.Tensor) -> tuple[torch.Tensor, torch.Tensor]:
    mse = torch.mean((prediction - target) ** 2, dim=(1, 2, 3))
    mse = torch.clamp(mse, min=1e-8)
    return -10.0 * torch.log10(mse), mse


def main() -> None:
    args = parse_args()
    checkpoint_path = Path(args.checkpoint)
    checkpoint, checkpoint_args = load_checkpoint_args(checkpoint_path)
    merged_args = merge_eval_args(args, checkpoint_args)

    if "dataset_format" not in merged_args or "data_path" not in merged_args:
        raise ValueError("dataset_format and data_path must be provided either via CLI or checkpoint args.")

    out_dir = Path(args.out_dir)
    vis_dir = out_dir / "vis"
    out_dir.mkdir(parents=True, exist_ok=True)
    vis_dir.mkdir(parents=True, exist_ok=True)

    device = pick_device(args.device)
    amp_dtype, _ = resolve_amp_mode(device, args.amp)
    dataset = build_dataset(argparse.Namespace(**merged_args))
    dataloader = DataLoader(
        dataset,
        batch_size=args.batch_size,
        shuffle=False,
        num_workers=args.workers,
        pin_memory=(device.type == "cuda"),
        drop_last=False,
        collate_fn=renderformer_baseline_collate,
    )

    config = build_baseline_config(
        latent_dim=int(checkpoint_args["latent_dim"]),
        num_layers=int(checkpoint_args["num_layers"]),
        num_heads=int(checkpoint_args["num_heads"]),
        view_layers=int(checkpoint_args["view_layers"]),
        view_num_heads=int(checkpoint_args["view_num_heads"]),
        use_dpt_decoder=bool(checkpoint_args.get("use_dpt_decoder", False)),
        num_register_tokens=int(checkpoint_args.get("num_register_tokens", 4)),
        patch_size=int(checkpoint_args.get("patch_size", 8)),
        texture_patch_size=int(checkpoint_args.get("texture_patch_size", 8)),
        vertex_pe_num_freqs=int(checkpoint_args.get("vertex_pe_num_freqs", 6)),
        vn_pe_num_freqs=int(checkpoint_args.get("vn_pe_num_freqs", 6)),
        use_vn_encoder=not bool(checkpoint_args.get("no_vn", False)),
        ffn_opt=str(checkpoint_args.get("ffn_opt", "checkpoint")),
    )
    model = CourseRenderFormerWrapper(config).to(device)
    model.load_state_dict(checkpoint["model_state_dict"])
    model.eval()

    per_sample_path = out_dir / "per_sample_metrics.jsonl"
    if per_sample_path.exists():
        per_sample_path.unlink()

    total_samples = 0
    psnr_clamped_sum = 0.0
    psnr_display_sum = 0.0
    mse_hdr_sum = 0.0
    mse_clamped_sum = 0.0
    saved_images = 0
    best_sample: Dict[str, Any] | None = None
    worst_sample: Dict[str, Any] | None = None

    with torch.no_grad():
        for batch in dataloader:
            batch = move_to_device(batch, device)
            ensure_resolution_is_valid(batch, config.patch_size)
            target = batch["gt_image"].to(dtype=torch.float32)

            autocast_context = (
                torch.autocast(device_type="cuda", dtype=amp_dtype)
                if device.type == "cuda" and amp_dtype is not None
                else nullcontext()
            )
            with autocast_context:
                prediction = model(batch)
            prediction = prediction.to(dtype=torch.float32)

            batch_psnr_clamped, batch_mse_clamped = compute_psnr(
                torch.clamp(prediction, 0.0, 1.0),
                torch.clamp(target, 0.0, 1.0),
            )
            batch_psnr_display, _ = compute_psnr(
                to_display_space(prediction),
                to_display_space(target),
            )
            batch_mse_hdr = torch.mean((prediction - target) ** 2, dim=(1, 2, 3))
            sample_names: List[str] = [str(item) for item in batch["sample_name"]]

            for sample_index, sample_name in enumerate(sample_names):
                sample_psnr_clamped = float(batch_psnr_clamped[sample_index].item())
                sample_psnr_display = float(batch_psnr_display[sample_index].item())
                sample_mse_hdr = float(batch_mse_hdr[sample_index].item())
                sample_mse_clamped = float(batch_mse_clamped[sample_index].item())
                record = {
                    "sample_name": sample_name,
                    "psnr_clamped": sample_psnr_clamped,
                    "psnr_display": sample_psnr_display,
                    "mse_hdr": sample_mse_hdr,
                    "mse_clamped": sample_mse_clamped,
                }
                append_jsonl(per_sample_path, record)

                if best_sample is None or sample_psnr_display > best_sample["psnr_display"]:
                    best_sample = record
                if worst_sample is None or sample_psnr_display < worst_sample["psnr_display"]:
                    worst_sample = record

                if saved_images < args.save_images:
                    save_preview(
                        prediction[sample_index:sample_index + 1],
                        target[sample_index:sample_index + 1],
                        vis_dir / f"sample_{saved_images:03d}_{sample_name.replace(':', '_')}.png",
                    )
                    saved_images += 1

            total_samples += len(sample_names)
            psnr_clamped_sum += float(batch_psnr_clamped.sum().item())
            psnr_display_sum += float(batch_psnr_display.sum().item())
            mse_hdr_sum += float(batch_mse_hdr.sum().item())
            mse_clamped_sum += float(batch_mse_clamped.sum().item())

    summary = {
        "checkpoint": str(checkpoint_path),
        "num_samples": total_samples,
        "mean_psnr_clamped": psnr_clamped_sum / max(total_samples, 1),
        "mean_psnr_display": psnr_display_sum / max(total_samples, 1),
        "mean_mse_hdr": mse_hdr_sum / max(total_samples, 1),
        "mean_mse_clamped": mse_clamped_sum / max(total_samples, 1),
        "dataset_format": merged_args["dataset_format"],
        "data_path": merged_args["data_path"],
        "saved_images": saved_images,
        "recommended_report_metric": "mean_psnr_clamped",
        "best_sample": best_sample,
        "worst_sample": worst_sample,
    }
    save_json(out_dir / "summary.json", summary)

    print(f"Checkpoint: {checkpoint_path}")
    print(f"Samples: {total_samples}")
    print(f"Mean PSNR (clamped): {summary['mean_psnr_clamped']:.4f} dB")
    print(f"Mean PSNR (display): {summary['mean_psnr_display']:.4f} dB")
    print(f"Mean MSE (HDR): {summary['mean_mse_hdr']:.8f}")


if __name__ == "__main__":
    main()