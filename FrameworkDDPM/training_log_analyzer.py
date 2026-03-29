import argparse
import csv
import json
import re
from collections import defaultdict
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np


TRAIN_LINE_RE = re.compile(
    r"INFO:root:Epoch (?P<epoch>\d+) \| "
    r"Batch index (?P<batch_index>\d+) \| "
    r"Step (?P<step>\d+) \| "
    r"Actual batch (?P<actual_batch>\d+) \| "
    r"Effective batch (?P<effective_batch>\d+) \| "
    r"Loss (?P<loss>[-+]?\d*\.?\d+)"
)

TEST_LINE_RE = re.compile(
    r"INFO:root:Epoch (?P<epoch>\d+) \| Test loss (?P<loss>[-+]?\d*\.?\d+)"
)

BEST_LINE_RE = re.compile(
    r"INFO:root:Saved best test-loss checkpoint to (?P<path>.+)"
)

METADATA_LINE_RE = re.compile(r"INFO:root:(?P<key>[^:]+): (?P<value>.+)")


def parse_args():
    parser = argparse.ArgumentParser(
        description="Clean and visualize DDPM training logs."
    )
    parser.add_argument(
        "--log-file",
        required=True,
        help="Path to the raw training log file.",
    )
    parser.add_argument(
        "--output-dir",
        default="",
        help="Directory for cleaned tables, summary JSON, and plots.",
    )
    parser.add_argument(
        "--smoothing-window",
        type=int,
        default=50,
        help="Moving average window used in smoothed plots.",
    )
    return parser.parse_args()


def normalize_metadata_key(text):
    normalized = text.strip().lower()
    normalized = re.sub(r"[^a-z0-9]+", "_", normalized)
    return normalized.strip("_")


def parse_log(log_path):
    metadata = {}
    train_batches = []
    test_losses = {}
    best_checkpoint_events = []
    last_test_epoch = None

    with Path(log_path).open("r", encoding="utf-8") as handle:
        for raw_line in handle:
            line = raw_line.strip()
            if not line:
                continue

            train_match = TRAIN_LINE_RE.match(line)
            if train_match:
                train_batches.append(
                    {
                        "epoch": int(train_match.group("epoch")),
                        "batch_index": int(train_match.group("batch_index")),
                        "step": int(train_match.group("step")),
                        "actual_batch": int(train_match.group("actual_batch")),
                        "effective_batch": int(
                            train_match.group("effective_batch")
                        ),
                        "loss": float(train_match.group("loss")),
                    }
                )
                continue

            test_match = TEST_LINE_RE.match(line)
            if test_match:
                epoch = int(test_match.group("epoch"))
                test_losses[epoch] = float(test_match.group("loss"))
                last_test_epoch = epoch
                continue

            best_match = BEST_LINE_RE.match(line)
            if best_match:
                best_checkpoint_events.append(
                    {
                        "epoch": last_test_epoch,
                        "checkpoint_path": best_match.group("path"),
                    }
                )
                continue

            metadata_match = METADATA_LINE_RE.match(line)
            if metadata_match:
                metadata[
                    normalize_metadata_key(metadata_match.group("key"))
                ] = metadata_match.group("value").strip()

    return metadata, train_batches, test_losses, best_checkpoint_events


def aggregate_epoch_metrics(train_batches, test_losses, best_events):
    grouped_train = defaultdict(list)
    for batch in train_batches:
        grouped_train[batch["epoch"]].append(batch)

    best_event_epochs = {
        event["epoch"] for event in best_events if event["epoch"] is not None
    }
    epochs = sorted(set(grouped_train) | set(test_losses))

    epoch_metrics = []
    best_test_so_far = None

    for epoch in epochs:
        batches = grouped_train.get(epoch, [])
        batch_losses = [batch["loss"] for batch in batches]
        train_loss_mean = (
            float(np.mean(batch_losses)) if batch_losses else None
        )
        train_loss_min = (
            float(np.min(batch_losses)) if batch_losses else None
        )
        train_loss_max = (
            float(np.max(batch_losses)) if batch_losses else None
        )
        test_loss = test_losses.get(epoch)

        if test_loss is not None:
            if best_test_so_far is None:
                best_test_so_far = test_loss
            else:
                best_test_so_far = min(best_test_so_far, test_loss)

        epoch_metrics.append(
            {
                "epoch": epoch,
                "train_batch_count": len(batches),
                "train_loss_mean": train_loss_mean,
                "train_loss_min": train_loss_min,
                "train_loss_max": train_loss_max,
                "test_loss": test_loss,
                "best_test_loss_so_far": best_test_so_far,
                "is_best_checkpoint_saved": epoch in best_event_epochs,
            }
        )

    return epoch_metrics


def write_csv(file_path, rows, fieldnames):
    with Path(file_path).open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            writer.writerow(row)


def moving_average(values, window):
    if len(values) == 0:
        return np.array([])
    if window <= 1 or len(values) < window:
        return np.asarray(values, dtype=float)
    kernel = np.ones(window, dtype=float) / float(window)
    return np.convolve(values, kernel, mode="valid")


def plot_loss_curves(epoch_metrics, output_path, smoothing_window):
    epochs = np.array([row["epoch"] for row in epoch_metrics], dtype=int)
    train_rows = [
        row for row in epoch_metrics if row["train_loss_mean"] is not None
    ]
    test_rows = [
        row for row in epoch_metrics if row["test_loss"] is not None
    ]
    best_rows = [
        row
        for row in epoch_metrics
        if row["best_test_loss_so_far"] is not None
    ]
    best_saved_rows = [
        row
        for row in epoch_metrics
        if row["is_best_checkpoint_saved"] and row["test_loss"] is not None
    ]
    train_epochs = np.array(
        [row["epoch"] for row in train_rows],
        dtype=int,
    )
    train_losses = np.array(
        [row["train_loss_mean"] for row in train_rows],
        dtype=float,
    )
    test_epochs = np.array(
        [row["epoch"] for row in test_rows],
        dtype=int,
    )
    test_losses = np.array(
        [row["test_loss"] for row in test_rows],
        dtype=float,
    )
    best_test_epochs = np.array(
        [row["epoch"] for row in best_rows],
        dtype=int,
    )
    best_test_losses = np.array(
        [row["best_test_loss_so_far"] for row in best_rows],
        dtype=float,
    )
    best_saved_epochs = np.array(
        [row["epoch"] for row in best_saved_rows],
        dtype=int,
    )
    best_saved_losses = np.array(
        [row["test_loss"] for row in best_saved_rows],
        dtype=float,
    )

    figure, axes = plt.subplots(2, 1, figsize=(12, 9), sharex=True)

    axes[0].plot(train_epochs, train_losses, label="Train loss", alpha=0.35)
    if len(test_losses) > 0:
        axes[0].plot(test_epochs, test_losses, label="Test loss", alpha=0.35)

    if len(train_losses) > 0:
        smoothed_train = moving_average(train_losses, smoothing_window)
        smoothed_train_epochs = train_epochs[
            len(train_epochs) - len(smoothed_train):
        ]
        axes[0].plot(
            smoothed_train_epochs,
            smoothed_train,
            label=f"Train MA({min(smoothing_window, len(train_losses))})",
            linewidth=2,
        )

    if len(test_losses) > 0:
        smoothed_test = moving_average(test_losses, smoothing_window)
        smoothed_test_epochs = test_epochs[
            len(test_epochs) - len(smoothed_test):
        ]
        axes[0].plot(
            smoothed_test_epochs,
            smoothed_test,
            label=f"Test MA({min(smoothing_window, len(test_losses))})",
            linewidth=2,
        )

    axes[0].set_ylabel("Loss")
    axes[0].set_title("Train/Test Loss Curves")
    axes[0].grid(alpha=0.25)
    axes[0].legend()

    if len(best_test_losses) > 0:
        axes[1].plot(
            best_test_epochs,
            best_test_losses,
            label="Best test loss so far",
            linewidth=2,
        )

    if len(best_saved_epochs) > 0:
        axes[1].scatter(
            best_saved_epochs,
            best_saved_losses,
            label="Best checkpoint saved",
            s=24,
            zorder=3,
        )

    axes[1].set_xlabel("Epoch")
    axes[1].set_ylabel("Loss")
    axes[1].set_title("Best Test-Loss Progress")
    axes[1].grid(alpha=0.25)
    axes[1].legend()

    if len(epochs) > 0:
        axes[1].set_xlim(int(epochs.min()), int(epochs.max()))

    figure.tight_layout()
    figure.savefig(output_path, dpi=160, bbox_inches="tight")
    plt.close(figure)


def build_summary(
    log_path,
    output_dir,
    metadata,
    train_batches,
    test_losses,
    best_events,
    epoch_metrics,
):
    observed_epochs = [row["epoch"] for row in epoch_metrics]
    best_test_row = None
    if epoch_metrics:
        valid_test_rows = [
            row for row in epoch_metrics if row["test_loss"] is not None
        ]
        if valid_test_rows:
            best_test_row = min(
                valid_test_rows,
                key=lambda row: row["test_loss"],
            )

    final_row = epoch_metrics[-1] if epoch_metrics else None
    train_losses = [batch["loss"] for batch in train_batches]

    return {
        "log_file": str(log_path),
        "output_dir": str(output_dir),
        "metadata": metadata,
        "observed_epoch_count": len(observed_epochs),
        "observed_epoch_min": (
            min(observed_epochs) if observed_epochs else None
        ),
        "observed_epoch_max": (
            max(observed_epochs) if observed_epochs else None
        ),
        "train_batch_count": len(train_batches),
        "test_epoch_count": len(test_losses),
        "best_checkpoint_event_count": len(best_events),
        "train_loss_min": min(train_losses) if train_losses else None,
        "train_loss_max": max(train_losses) if train_losses else None,
        "train_loss_final": (
            final_row["train_loss_mean"] if final_row else None
        ),
        "test_loss_final": final_row["test_loss"] if final_row else None,
        "best_test_loss": (
            best_test_row["test_loss"] if best_test_row else None
        ),
        "best_test_epoch": best_test_row["epoch"] if best_test_row else None,
        "best_test_loss_so_far_final": (
            final_row["best_test_loss_so_far"] if final_row else None
        ),
    }


def main():
    args = parse_args()
    log_path = Path(args.log_file).resolve()
    output_dir = (
        Path(args.output_dir).resolve()
        if args.output_dir
        else log_path.with_name(f"{log_path.stem}_analysis")
    )
    output_dir.mkdir(parents=True, exist_ok=True)

    metadata, train_batches, test_losses, best_events = parse_log(log_path)
    epoch_metrics = aggregate_epoch_metrics(
        train_batches,
        test_losses,
        best_events,
    )

    write_csv(
        output_dir / "training_batches.csv",
        train_batches,
        [
            "epoch",
            "batch_index",
            "step",
            "actual_batch",
            "effective_batch",
            "loss",
        ],
    )
    write_csv(
        output_dir / "epoch_metrics.csv",
        epoch_metrics,
        [
            "epoch",
            "train_batch_count",
            "train_loss_mean",
            "train_loss_min",
            "train_loss_max",
            "test_loss",
            "best_test_loss_so_far",
            "is_best_checkpoint_saved",
        ],
    )

    (output_dir / "metadata.json").write_text(
        json.dumps(metadata, indent=2, ensure_ascii=False),
        encoding="utf-8",
    )

    summary = build_summary(
        log_path,
        output_dir,
        metadata,
        train_batches,
        test_losses,
        best_events,
        epoch_metrics,
    )
    (output_dir / "summary.json").write_text(
        json.dumps(summary, indent=2, ensure_ascii=False),
        encoding="utf-8",
    )

    plot_loss_curves(
        epoch_metrics,
        output_dir / "loss_curves.png",
        max(1, args.smoothing_window),
    )

    print(json.dumps(summary, indent=2, ensure_ascii=False))
    print(output_dir)


if __name__ == "__main__":
    main()
