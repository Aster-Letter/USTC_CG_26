import argparse
import logging
import math
from pathlib import Path
from typing import Sized, cast

import torch
import torch.nn.functional as F
from torch.amp.autocast_mode import autocast
from torch.amp.grad_scaler import GradScaler
from torch.optim import Adam

from dataloader import load_transformed_dataset
from forward_noising import forward_diffusion_sample
from unet import SimpleUnet

logging.basicConfig(level=logging.INFO)


T = 300
IMG_SIZE = 256
BATCH_SIZE = 1
EPOCHS = 2500
LEARNING_RATE = 1e-4
LOG_INTERVAL = 50
CHECKPOINT_INTERVAL = 500
DEFAULT_DATASET_CANDIDATES = [
    "datasets-1",
    "datasets-2",
    "datasets-icon-clean",
    "datasets-icon-clean-origin",
]


def resolve_default_datasets(project_root):
    """优先使用预期目录；若目录已漂移，则回退到现有 datasets-*。"""
    preferred = [
        name
        for name in DEFAULT_DATASET_CANDIDATES
        if (project_root / name).exists()
    ]
    if preferred:
        return preferred

    discovered = sorted(
        path.name
        for path in project_root.iterdir()
        if path.is_dir()
        and path.name.startswith("datasets-")
        and "-mini" not in path.name
        and "test" not in path.name
    )
    if discovered:
        return discovered

    raise FileNotFoundError(
        "FrameworkDDPM 下未发现可用数据集目录，请先准备 datasets-* 目录。"
    )


def parse_args():
    """解析训练脚本参数，便于按数据集/轮数灵活启动训练。"""
    project_root = Path(__file__).resolve().parent
    default_datasets = resolve_default_datasets(project_root)
    parser = argparse.ArgumentParser(
        description="Train DDPM on one or more datasets."
    )
    parser.add_argument(
        "--datasets",
        nargs="+",
        default=default_datasets,
        help="One or more dataset directories under FrameworkDDPM.",
    )
    parser.add_argument(
        "--epochs",
        type=int,
        default=EPOCHS,
        help="Number of training epochs.",
    )
    parser.add_argument(
        "--img-size",
        type=int,
        default=IMG_SIZE,
        help="Training image size after resize.",
    )
    parser.add_argument(
        "--batch-size",
        type=int,
        default=BATCH_SIZE,
        help="Training batch size.",
    )
    parser.add_argument(
        "--effective-batch-size",
        type=int,
        default=0,
        help=(
            "Target batch size after repeating samples inside a tiny batch. "
            "Useful when the dataset is too small for DataLoader to fill the "
            "requested batch size. Set 0 to keep the original batch."
        ),
    )
    parser.add_argument(
        "--learning-rate",
        type=float,
        default=LEARNING_RATE,
        help="Optimizer learning rate.",
    )
    parser.add_argument(
        "--log-interval",
        type=int,
        default=LOG_INTERVAL,
        help="How many batches between logging lines.",
    )
    parser.add_argument(
        "--num-workers",
        type=int,
        default=0,
        help="Number of DataLoader workers.",
    )
    parser.add_argument(
        "--checkpoint-interval",
        type=int,
        default=CHECKPOINT_INTERVAL,
        help="How many epochs between intermediate checkpoints.",
    )
    parser.add_argument(
        "--max-train-steps",
        type=int,
        default=0,
        help=(
            "Optional maximum optimizer steps. Useful for tiny datasets where "
            "epoch count no longer reflects the real training budget."
        ),
    )
    parser.add_argument(
        "--save-path",
        type=str,
        default="",
        help=(
            "Optional output checkpoint path. "
            "Defaults to ddpm_mse_epochs_<epochs>.pth."
        ),
    )
    parser.add_argument(
        "--eval-on-test",
        action="store_true",
        help="Evaluate average loss on the test split after each epoch.",
    )
    parser.add_argument(
        "--save-best-on-test",
        action="store_true",
        help=(
            "Save an extra best-test-loss checkpoint "
            "when eval-on-test is enabled."
        ),
    )
    parser.add_argument(
        "--resume-from",
        type=str,
        default="",
        help="Optional checkpoint path to load model weights before training.",
    )
    parser.add_argument(
        "--grad-clip",
        type=float,
        default=0.0,
        help="Optional gradient clipping max norm. Set 0 to disable.",
    )
    parser.add_argument(
        "--disable-amp",
        action="store_true",
        help="Disable CUDA automatic mixed precision training.",
    )
    return parser.parse_args()


def select_device():
    """优先选择 CUDA；若不可用则回退到 CPU。"""
    if torch.cuda.is_available():
        return torch.device("cuda")
    return torch.device("cpu")


def get_loss(model, x_0, t, device):
    """执行一次 DDPM 训练损失计算。

    输入：
    - x_0: 原始图像
    - t: 每个样本各自随机采样的时间步

    过程：
    1. 使用前向扩散得到 x_t 和真实噪声 epsilon
    2. 让 U-Net 根据 x_t 和 t 预测噪声
    3. 用 MSE 约束预测噪声逼近真实噪声

    输出：
    - 标量 loss，用于反向传播更新模型参数
    """
    # 第一步：按时间步 t 把原图 x_0 扩散成带噪图像 x_t。
    x_noisy, noise = forward_diffusion_sample(x_0, t, device)

    # 第二步：模型学习“看到 x_t 后把噪声 epsilon 预测出来”。
    predicted_noise = model(x_noisy, t)

    # 第三步：DDPM 的基础训练目标就是最小化噪声预测误差。
    return F.mse_loss(predicted_noise, noise)


def load_checkpoint_weights(model, checkpoint_path, device):
    """兼容仅模型权重的旧 checkpoint 格式。"""
    checkpoint_path = Path(checkpoint_path)
    state = torch.load(checkpoint_path, map_location=device)
    if isinstance(state, dict) and "state_dict" in state:
        state = state["state_dict"]
    model.load_state_dict(state)
    return checkpoint_path


def repeat_batch_to_size(batch, target_batch_size):
    """在极小数据集场景下重复当前 batch，确保一次更新能覆盖更多时间步。"""
    current_batch_size = batch.shape[0]
    if target_batch_size <= current_batch_size:
        return batch

    repeat_factor = math.ceil(target_batch_size / current_batch_size)
    repeat_dims = (repeat_factor,) + (1,) * (batch.dim() - 1)
    repeated = batch.repeat(repeat_dims)
    return repeated[:target_batch_size]


@torch.no_grad()
def evaluate_average_loss(model, dataloader, device):
    """在指定 dataloader 上估计平均噪声预测误差。"""
    model.eval()
    total_loss = 0.0
    total_batches = 0

    for batch, _ in dataloader:
        x_0 = batch.to(device, non_blocking=device.type == "cuda")
        t = torch.randint(
            0,
            T,
            (x_0.shape[0],),
            device=device,
            dtype=torch.long,
        )
        loss = get_loss(model, x_0, t, device)
        total_loss += loss.item()
        total_batches += 1

    model.train()
    if total_batches == 0:
        return None
    return total_loss / total_batches


if __name__ == "__main__":
    args = parse_args()
    project_root = Path(__file__).resolve().parent
    model_root = project_root / "model"
    model = SimpleUnet()
    save_path = (
        Path(args.save_path)
        if args.save_path
        else model_root / f"ddpm_mse_epochs_{args.epochs}.pth"
    )
    save_path.parent.mkdir(parents=True, exist_ok=True)

    device = select_device()
    use_cuda = device.type == "cuda"
    use_amp = use_cuda and not args.disable_amp

    if use_cuda:
        # 卷积输入尺寸固定时，benchmark 往往能为 GPU 找到更快的实现。
        torch.backends.cudnn.benchmark = True
        # 在 Ada/Lovelace 等新卡上，适度放宽 float32 matmul 精度通常能换来更高吞吐。
        torch.set_float32_matmul_precision("high")

    dataloader = load_transformed_dataset(
        img_size=args.img_size,
        batch_size=args.batch_size,
        dataset_names=args.datasets,
        split="train",
        num_workers=args.num_workers,
        pin_memory=use_cuda,
    )

    eval_dataloader = None
    if args.eval_on_test:
        eval_dataloader = load_transformed_dataset(
            img_size=args.img_size,
            batch_size=args.batch_size,
            dataset_names=args.datasets,
            split="test",
            num_workers=args.num_workers,
            pin_memory=use_cuda,
            shuffle=False,
        )

    dataset_size = len(cast(Sized, dataloader.dataset))
    steps_per_epoch = len(dataloader)
    effective_batch_size = (
        args.effective_batch_size
        if args.effective_batch_size > 0
        else args.batch_size
    )
    estimated_optimizer_steps = (
        args.max_train_steps
        if args.max_train_steps > 0
        else args.epochs * steps_per_epoch
    )
    estimated_timestep_draws = estimated_optimizer_steps * effective_batch_size

    logging.info("Using device: %s", device)
    if use_cuda:
        logging.info("GPU: %s", torch.cuda.get_device_name(0))
    logging.info("Dataset roots: %s", ", ".join(args.datasets))
    logging.info("Total training images: %d", dataset_size)
    logging.info("Steps per epoch: %d", steps_per_epoch)
    logging.info("Epochs: %d", args.epochs)
    logging.info("Image size: %d", args.img_size)
    logging.info("Batch size: %d", args.batch_size)
    logging.info("Effective batch size: %d", effective_batch_size)
    logging.info("Learning rate: %.6f", args.learning_rate)
    logging.info("AMP enabled: %s", use_amp)
    logging.info("Estimated optimizer steps: %d", estimated_optimizer_steps)
    logging.info(
        "Estimated timestep draws over training: %d (T=%d)",
        estimated_timestep_draws,
        T,
    )
    logging.info("Final save path: %s", save_path)

    if dataset_size == 1 and effective_batch_size <= 1:
        logging.warning(
            "单图数据集当前每次优化仅使用 1 张样本、1 个时间步；"
            "建议设置 --effective-batch-size 8 或更高，并配合 --max-train-steps。"
        )
    elif dataset_size < args.batch_size:
        logging.warning(
            "数据集大小 %d 小于请求 batch size %d；DataLoader 实际返回的 batch 会更小。",
            dataset_size,
            args.batch_size,
        )

    model.to(device)
    if args.resume_from:
        resumed_path = load_checkpoint_weights(model, args.resume_from, device)
        logging.info("Resumed model weights from: %s", resumed_path)

    model.train()
    optimizer = Adam(model.parameters(), lr=args.learning_rate)
    scaler = GradScaler(device="cuda", enabled=use_amp)
    best_eval_loss = None
    global_step = 0

    for epoch in range(args.epochs):
        for batch_idx, (batch, _) in enumerate(dataloader):
            # set_to_none=True 可以减少不必要的显存写回，训练时更高效。
            optimizer.zero_grad(set_to_none=True)

            # 每个 batch 中的每张图像都独立采样一个时间步，
            # 这样模型在训练过程中能覆盖从 0 到 T-1 的不同噪声强度。
            x_0 = batch.to(device, non_blocking=use_cuda)
            actual_batch_size = x_0.shape[0]
            x_0 = repeat_batch_to_size(x_0, effective_batch_size)
            batch_size = x_0.shape[0]
            t = torch.randint(
                0,
                T,
                (batch_size,),
                device=device,
                dtype=torch.long,
            )

            # 计算 loss -> 反向传播 -> 更新参数，构成一次完整训练迭代。
            with autocast(
                device_type=device.type,
                enabled=use_amp,
            ):
                loss = get_loss(model, x_0, t, device)

            scaler.scale(loss).backward()

            if args.grad_clip > 0:
                scaler.unscale_(optimizer)
                torch.nn.utils.clip_grad_norm_(
                    model.parameters(),
                    args.grad_clip,
                )

            scaler.step(optimizer)
            scaler.update()
            global_step += 1

            if batch_idx % args.log_interval == 0:
                logging.info(
                    (
                        "Epoch %d | Batch index %03d | Step %d | "
                        "Actual batch %d | Effective batch %d | Loss %.6f"
                    ),
                    epoch,
                    batch_idx,
                    global_step,
                    actual_batch_size,
                    batch_size,
                    loss.item(),
                )

            if (
                args.max_train_steps > 0
                and global_step >= args.max_train_steps
            ):
                break

        # 数据集变大后，一次完整训练会明显更久；周期性保存中间权重可以降低意外中断的代价。
        should_save_checkpoint = (
            args.checkpoint_interval > 0
            and (epoch + 1) % args.checkpoint_interval == 0
        )
        if should_save_checkpoint:
            checkpoint_path = save_path.with_name(
                f"{save_path.stem}_epoch_{epoch + 1}{save_path.suffix}"
            )
            torch.save(model.state_dict(), checkpoint_path)
            logging.info(
                "Saved intermediate checkpoint to %s",
                checkpoint_path,
            )

        if eval_dataloader is not None:
            eval_loss = evaluate_average_loss(model, eval_dataloader, device)
            if eval_loss is not None:
                logging.info(
                    "Epoch %d | Test loss %.6f",
                    epoch,
                    eval_loss,
                )

                should_save_best = (
                    args.save_best_on_test
                    and (
                        best_eval_loss is None
                        or eval_loss < best_eval_loss
                    )
                )
                if should_save_best:
                    best_eval_loss = eval_loss
                    best_path = save_path.with_name(
                        f"{save_path.stem}_best{save_path.suffix}"
                    )
                    torch.save(model.state_dict(), best_path)
                    logging.info(
                        "Saved best test-loss checkpoint to %s",
                        best_path,
                    )

        if (
            args.max_train_steps > 0
            and global_step >= args.max_train_steps
        ):
            logging.info(
                "Reached max train steps: %d",
                args.max_train_steps,
            )
            break

    torch.save(model.state_dict(), save_path)
