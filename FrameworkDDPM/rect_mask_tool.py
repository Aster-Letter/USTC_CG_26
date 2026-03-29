import argparse
from pathlib import Path

import cv2 as cv
import numpy as np


MASK_PRESETS = {
    "center-small": {
        "unit": "ratio",
        "x": 0.35,
        "y": 0.35,
        "width": 0.30,
        "height": 0.30,
    },
    "center-medium": {
        "unit": "ratio",
        "x": 0.25,
        "y": 0.25,
        "width": 0.50,
        "height": 0.50,
    },
    "center-large": {
        "unit": "ratio",
        "x": 0.15,
        "y": 0.15,
        "width": 0.70,
        "height": 0.70,
    },
    "center-wide": {
        "unit": "ratio",
        "x": 0.15,
        "y": 0.30,
        "width": 0.70,
        "height": 0.40,
    },
    "center-tall": {
        "unit": "ratio",
        "x": 0.30,
        "y": 0.15,
        "width": 0.40,
        "height": 0.70,
    },
    "top-left": {
        "unit": "ratio",
        "x": 0.00,
        "y": 0.00,
        "width": 0.40,
        "height": 0.40,
    },
    "top-right": {
        "unit": "ratio",
        "x": 0.60,
        "y": 0.00,
        "width": 0.40,
        "height": 0.40,
    },
    "bottom-left": {
        "unit": "ratio",
        "x": 0.00,
        "y": 0.60,
        "width": 0.40,
        "height": 0.40,
    },
    "bottom-right": {
        "unit": "ratio",
        "x": 0.60,
        "y": 0.60,
        "width": 0.40,
        "height": 0.40,
    },
    "left-band": {
        "unit": "ratio",
        "x": 0.00,
        "y": 0.15,
        "width": 0.35,
        "height": 0.70,
    },
    "right-band": {
        "unit": "ratio",
        "x": 0.65,
        "y": 0.15,
        "width": 0.35,
        "height": 0.70,
    },
    "top-band": {
        "unit": "ratio",
        "x": 0.15,
        "y": 0.00,
        "width": 0.70,
        "height": 0.35,
    },
    "bottom-band": {
        "unit": "ratio",
        "x": 0.15,
        "y": 0.65,
        "width": 0.70,
        "height": 0.35,
    },
}


def format_mask_presets():
    lines = []
    for preset_name in sorted(MASK_PRESETS):
        preset = MASK_PRESETS[preset_name]
        lines.append(
            f"{preset_name}: unit={preset['unit']} "
            f"x={preset['x']} y={preset['y']} "
            f"width={preset['width']} height={preset['height']}"
        )
    return "\n".join(lines)


def parse_args():
    parser = argparse.ArgumentParser(
        description=(
            "Generate a white-keep black-hole rectangular inpainting mask "
            "and an optional overlay preview."
        )
    )
    parser.add_argument(
        "--input-image",
        default="",
        help="Source image used to infer mask size and create a preview.",
    )
    parser.add_argument(
        "--output-mask",
        default="",
        help="Output path of the generated mask image.",
    )
    parser.add_argument(
        "--output-preview",
        default="",
        help=(
            "Optional preview image path with a red translucent "
            "hole overlay."
        ),
    )
    parser.add_argument(
        "--unit",
        choices=["ratio", "pixel"],
        default="ratio",
        help="Interpret rectangle values as ratios or pixels.",
    )
    parser.add_argument(
        "--preset",
        choices=sorted(MASK_PRESETS),
        default="",
        help="Use a built-in rectangle preset instead of manual geometry.",
    )
    parser.add_argument(
        "--list-presets",
        action="store_true",
        help="Print all built-in mask presets and exit.",
    )
    parser.add_argument(
        "--x",
        type=float,
        default=0.25,
        help="Left position of the rectangle.",
    )
    parser.add_argument(
        "--y",
        type=float,
        default=0.25,
        help="Top position of the rectangle.",
    )
    parser.add_argument(
        "--width",
        type=float,
        default=0.5,
        help="Rectangle width.",
    )
    parser.add_argument(
        "--height",
        type=float,
        default=0.5,
        help="Rectangle height.",
    )
    return parser.parse_args()


def validate_args(args):
    if args.list_presets:
        return
    if not args.input_image:
        raise ValueError(
            "--input-image is required unless --list-presets is used."
        )
    if not args.output_mask:
        raise ValueError(
            "--output-mask is required unless --list-presets is used."
        )


def load_image(image_path):
    image_path = Path(image_path)
    image = cv.imread(str(image_path))
    if image is None:
        raise FileNotFoundError(f"Failed to read input image: {image_path}")
    return image_path, image


def resolve_rectangle_values(
    unit,
    x,
    y,
    width,
    height,
    image_width,
    image_height,
):
    if unit == "ratio":
        left = int(round(x * image_width))
        top = int(round(y * image_height))
        width = int(round(width * image_width))
        height = int(round(height * image_height))
    else:
        left = int(round(x))
        top = int(round(y))
        width = int(round(width))
        height = int(round(height))

    width = max(1, width)
    height = max(1, height)
    left = max(0, min(left, image_width - 1))
    top = max(0, min(top, image_height - 1))
    right = max(left + 1, min(left + width, image_width))
    bottom = max(top + 1, min(top + height, image_height))
    return left, top, right, bottom


def resolve_preset_rectangle(preset_name, image_width, image_height):
    preset = MASK_PRESETS[preset_name]
    return resolve_rectangle_values(
        preset["unit"],
        preset["x"],
        preset["y"],
        preset["width"],
        preset["height"],
        image_width,
        image_height,
    )


def resolve_rectangle(args, image_width, image_height):
    if args.preset:
        return resolve_preset_rectangle(
            args.preset,
            image_width,
            image_height,
        )
    return resolve_rectangle_values(
        args.unit,
        args.x,
        args.y,
        args.width,
        args.height,
        image_width,
        image_height,
    )


def create_mask(image_shape, rectangle):
    image_height, image_width = image_shape[:2]
    left, top, right, bottom = rectangle
    mask = np.full((image_height, image_width), 255, dtype=np.uint8)
    mask[top:bottom, left:right] = 0
    return mask


def create_preview(image_bgr, rectangle):
    left, top, right, bottom = rectangle
    preview = image_bgr.copy()
    overlay = preview.copy()
    cv.rectangle(
        overlay,
        (left, top),
        (right, bottom),
        (0, 0, 255),
        thickness=-1,
    )
    preview = cv.addWeighted(overlay, 0.28, preview, 0.72, 0)
    cv.rectangle(
        preview,
        (left, top),
        (right, bottom),
        (0, 0, 255),
        thickness=2,
    )
    return preview


def main():
    args = parse_args()
    if args.list_presets:
        print(format_mask_presets())
        return

    validate_args(args)
    _, image_bgr = load_image(args.input_image)
    image_height, image_width = image_bgr.shape[:2]
    rectangle = resolve_rectangle(args, image_width, image_height)

    mask = create_mask(image_bgr.shape, rectangle)
    output_mask = Path(args.output_mask)
    output_mask.parent.mkdir(parents=True, exist_ok=True)
    cv.imwrite(str(output_mask), mask)

    print(f"mask_path={output_mask}")
    print(
        "rectangle="
        f"left:{rectangle[0]},top:{rectangle[1]},"
        f"right:{rectangle[2]},bottom:{rectangle[3]}"
    )
    if args.preset:
        print(f"preset={args.preset}")

    if args.output_preview:
        preview = create_preview(image_bgr, rectangle)
        output_preview = Path(args.output_preview)
        output_preview.parent.mkdir(parents=True, exist_ok=True)
        cv.imwrite(str(output_preview), preview)
        print(f"preview_path={output_preview}")


if __name__ == "__main__":
    main()
