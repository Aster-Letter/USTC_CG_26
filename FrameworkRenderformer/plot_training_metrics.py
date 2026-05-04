from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Dict, List

from PIL import Image, ImageDraw, ImageFont


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Plot course baseline training metrics from metrics.jsonl without matplotlib.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument("--metrics", type=str, required=True, help="Path to metrics.jsonl")
    parser.add_argument("--out", type=str, required=True, help="Output PNG path")
    parser.add_argument("--fields", nargs="+", default=["total_loss", "base_loss"], help="Metric fields to draw")
    parser.add_argument("--title", type=str, default="Training Metrics")
    parser.add_argument("--width", type=int, default=1280)
    parser.add_argument("--height", type=int, default=720)
    return parser.parse_args()


def load_records(metrics_path: Path) -> List[Dict[str, float]]:
    records: List[Dict[str, float]] = []
    with metrics_path.open("r", encoding="utf-8") as file:
        for line in file:
            line = line.strip()
            if not line:
                continue
            records.append(json.loads(line))
    if not records:
        raise ValueError(f"No metric records found in {metrics_path}")
    return records


def normalize(value: float, lo: float, hi: float) -> float:
    if hi <= lo:
        return 0.5
    return (value - lo) / (hi - lo)


def main() -> None:
    args = parse_args()
    metrics_path = Path(args.metrics)
    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)

    records = load_records(metrics_path)
    steps = [int(record["step"]) for record in records]

    available_fields = [field for field in args.fields if all(field in record for record in records)]
    if not available_fields:
        raise KeyError(f"None of the requested fields are present: {args.fields}")

    color_palette = [
        (201, 60, 32),
        (32, 101, 164),
        (78, 154, 6),
        (117, 80, 123),
        (196, 160, 0),
    ]

    margin_left = 90
    margin_right = 40
    margin_top = 80
    margin_bottom = 90
    plot_width = args.width - margin_left - margin_right
    plot_height = args.height - margin_top - margin_bottom

    all_values = [float(record[field]) for field in available_fields for record in records]
    y_min = min(all_values)
    y_max = max(all_values)
    if y_min == y_max:
        padding = max(abs(y_min) * 0.1, 1e-3)
        y_min -= padding
        y_max += padding
    else:
        padding = (y_max - y_min) * 0.08
        y_min -= padding
        y_max += padding

    x_min = min(steps)
    x_max = max(steps)

    image = Image.new("RGB", (args.width, args.height), (250, 248, 243))
    draw = ImageDraw.Draw(image)
    font = ImageFont.load_default()

    plot_left = margin_left
    plot_top = margin_top
    plot_right = plot_left + plot_width
    plot_bottom = plot_top + plot_height

    draw.rectangle([plot_left, plot_top, plot_right, plot_bottom], outline=(80, 80, 80), width=2)

    for tick_idx in range(6):
        y = plot_top + plot_height * tick_idx / 5
        draw.line([(plot_left, y), (plot_right, y)], fill=(220, 220, 220), width=1)
        y_value = y_max - (y_max - y_min) * tick_idx / 5
        draw.text((16, y - 6), f"{y_value:.4f}", fill=(60, 60, 60), font=font)

    x_tick_count = min(6, max(2, len(steps)))
    for tick_idx in range(x_tick_count):
        x = plot_left + plot_width * tick_idx / (x_tick_count - 1)
        draw.line([(x, plot_top), (x, plot_bottom)], fill=(230, 230, 230), width=1)
        step_pos = tick_idx / (x_tick_count - 1)
        step_value = round(x_min + (x_max - x_min) * step_pos)
        draw.text((x - 12, plot_bottom + 12), str(step_value), fill=(60, 60, 60), font=font)

    for field_idx, field in enumerate(available_fields):
        color = color_palette[field_idx % len(color_palette)]
        points = []
        for record in records:
            x_norm = normalize(float(record["step"]), float(x_min), float(x_max))
            y_norm = normalize(float(record[field]), y_min, y_max)
            px = plot_left + x_norm * plot_width
            py = plot_bottom - y_norm * plot_height
            points.append((px, py))

        if len(points) == 1:
            px, py = points[0]
            draw.ellipse([px - 4, py - 4, px + 4, py + 4], fill=color, outline=color)
        else:
            draw.line(points, fill=color, width=3)
            for px, py in points:
                draw.ellipse([px - 3, py - 3, px + 3, py + 3], fill=color, outline=color)

        legend_x = plot_left + 12
        legend_y = 18 + field_idx * 18
        draw.line([(legend_x, legend_y + 6), (legend_x + 20, legend_y + 6)], fill=color, width=3)
        draw.text((legend_x + 28, legend_y), field, fill=(40, 40, 40), font=font)

    draw.text((plot_left, 18), args.title, fill=(20, 20, 20), font=font)
    draw.text((plot_left, args.height - 28), "step", fill=(40, 40, 40), font=font)
    draw.text((16, plot_top - 18), "metric value", fill=(40, 40, 40), font=font)

    image.save(out_path)
    print(f"Saved plot to {out_path}")


if __name__ == "__main__":
    main()