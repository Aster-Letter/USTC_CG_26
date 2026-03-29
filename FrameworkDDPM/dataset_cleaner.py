import argparse
import hashlib
import json
import random
import shutil
from dataclasses import asdict, dataclass
from pathlib import Path

from PIL import Image, UnidentifiedImageError


VALID_SUFFIXES = {".png", ".jpg", ".jpeg", ".bmp", ".webp"}


@dataclass
class ImageRecord:
    source_path: str
    sha256: str
    width: int
    height: int


def parse_args():
    parser = argparse.ArgumentParser(
        description=(
            "Clean, deduplicate, rename and split images "
            "for DDPM datasets."
        )
    )
    parser.add_argument(
        "--input-dir",
        required=True,
        help="Directory containing raw images or subfolders of raw images.",
    )
    parser.add_argument(
        "--output-dir",
        required=True,
        help=(
            "Dataset output directory, "
            "e.g. FrameworkDDPM/datasets-icon-clean."
        ),
    )
    parser.add_argument(
        "--class-prefix",
        default="cls",
        help="Class folder prefix used under train/ and test/.",
    )
    parser.add_argument(
        "--prefix",
        default="image",
        help="Renamed file prefix, e.g. icon -> icon_0001.png.",
    )
    parser.add_argument(
        "--test-ratio",
        type=float,
        default=0.2,
        help="Fraction of cleaned images to place into test split.",
    )
    parser.add_argument(
        "--min-size",
        type=int,
        default=32,
        help="Minimum accepted width and height in pixels.",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=3407,
        help="Random seed for deterministic split.",
    )
    parser.add_argument(
        "--copy",
        action="store_true",
        help="Copy images instead of moving them. Default behavior is copy.",
    )
    parser.add_argument(
        "--move",
        action="store_true",
        help="Move images into the output dataset instead of copying.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Preview the cleaning result without writing files.",
    )
    parser.add_argument(
        "--manifest-name",
        default="cleaning_manifest.json",
        help="Manifest file name written under the output directory.",
    )
    return parser.parse_args()


def validate_paths(args):
    input_root = Path(args.input_dir).resolve()
    output_root = Path(args.output_dir).resolve()

    if args.copy and args.move:
        raise ValueError("Choose either --copy or --move, not both.")

    if not input_root.exists():
        raise FileNotFoundError(
            f"Input directory does not exist: {input_root}"
        )

    if input_root == output_root:
        raise ValueError("Input and output directories must be different.")

    if output_root.is_relative_to(input_root):
        raise ValueError(
            "Output directory cannot be placed inside input directory. "
            "Use a sibling directory instead."
        )


def collect_image_paths(input_dir):
    input_root = Path(input_dir)
    image_paths = []
    for path in input_root.rglob("*"):
        if path.is_file() and path.suffix.lower() in VALID_SUFFIXES:
            image_paths.append(path)
    return sorted(image_paths)


def hash_file(file_path):
    digest = hashlib.sha256()
    with file_path.open("rb") as file_handle:
        for chunk in iter(lambda: file_handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def validate_image(image_path, min_size):
    try:
        with Image.open(image_path) as image:
            image.verify()

        with Image.open(image_path) as image:
            width, height = image.size
    except (UnidentifiedImageError, OSError, ValueError):
        return None

    if width < min_size or height < min_size:
        return None

    return ImageRecord(
        source_path=str(image_path),
        sha256=hash_file(image_path),
        width=width,
        height=height,
    )


def clean_records(image_paths, min_size):
    valid_records = []
    invalid_paths = []
    duplicate_paths = []
    seen_hashes = {}

    for image_path in image_paths:
        record = validate_image(image_path, min_size)
        if record is None:
            invalid_paths.append(str(image_path))
            continue

        if record.sha256 in seen_hashes:
            duplicate_paths.append(str(image_path))
            continue

        seen_hashes[record.sha256] = str(image_path)
        valid_records.append(record)

    return valid_records, invalid_paths, duplicate_paths


def split_records(records, test_ratio, seed):
    shuffled_records = list(records)
    random.Random(seed).shuffle(shuffled_records)

    test_count = int(round(len(shuffled_records) * test_ratio))
    if len(shuffled_records) > 1:
        test_count = max(1, min(test_count, len(shuffled_records) - 1))
    else:
        test_count = 0

    test_records = shuffled_records[:test_count]
    train_records = shuffled_records[test_count:]
    return train_records, test_records


def ensure_output_dirs(output_dir):
    output_root = Path(output_dir)
    train_dir = output_root / "train"
    test_dir = output_root / "test"
    train_dir.mkdir(parents=True, exist_ok=True)
    test_dir.mkdir(parents=True, exist_ok=True)
    return output_root, train_dir, test_dir


def write_split(
    records,
    destination_dir,
    prefix,
    class_prefix,
    start_index,
    move_files,
):
    written_paths = []
    for offset, record in enumerate(records, start=start_index):
        source_path = Path(record.source_path)
        class_dir = destination_dir / f"{class_prefix}{offset:04d}"
        class_dir.mkdir(parents=True, exist_ok=True)
        new_name = f"{prefix}_{offset:04d}{source_path.suffix.lower()}"
        destination_path = class_dir / new_name
        if move_files:
            shutil.move(str(source_path), str(destination_path))
        else:
            shutil.copy2(str(source_path), str(destination_path))
        written_paths.append(str(destination_path))
    return written_paths


def build_manifest(
    args,
    image_paths,
    valid_records,
    invalid_paths,
    duplicate_paths,
    train_records,
    test_records,
    written_train,
    written_test,
):
    return {
        "input_dir": str(Path(args.input_dir).resolve()),
        "output_dir": str(Path(args.output_dir).resolve()),
        "class_prefix": args.class_prefix,
        "prefix": args.prefix,
        "transfer_mode": "move" if args.move else "copy",
        "test_ratio": args.test_ratio,
        "seed": args.seed,
        "min_size": args.min_size,
        "source_count": len(image_paths),
        "valid_unique_count": len(valid_records),
        "invalid_count": len(invalid_paths),
        "duplicate_count": len(duplicate_paths),
        "train_count": len(train_records),
        "test_count": len(test_records),
        "invalid_paths": invalid_paths,
        "duplicate_paths": duplicate_paths,
        "train_outputs": written_train,
        "test_outputs": written_test,
        "records": [asdict(record) for record in valid_records],
    }


def print_summary(manifest, dry_run):
    mode_text = "dry-run" if dry_run else "write"
    print(f"mode={mode_text}")
    print(f"source_count={manifest['source_count']}")
    print(f"valid_unique_count={manifest['valid_unique_count']}")
    print(f"invalid_count={manifest['invalid_count']}")
    print(f"duplicate_count={manifest['duplicate_count']}")
    print(f"train_count={manifest['train_count']}")
    print(f"test_count={manifest['test_count']}")
    print(f"output_dir={manifest['output_dir']}")


def main():
    args = parse_args()
    validate_paths(args)
    move_files = args.move

    image_paths = collect_image_paths(args.input_dir)
    if not image_paths:
        raise FileNotFoundError(
            "No supported images found under: "
            f"{Path(args.input_dir).resolve()}"
        )

    valid_records, invalid_paths, duplicate_paths = clean_records(
        image_paths,
        args.min_size,
    )
    if not valid_records:
        raise RuntimeError("No valid images remain after cleaning.")

    train_records, test_records = split_records(
        valid_records,
        args.test_ratio,
        args.seed,
    )

    written_train = []
    written_test = []
    output_root = Path(args.output_dir)

    if not args.dry_run:
        output_root, train_dir, test_dir = ensure_output_dirs(
            args.output_dir,
        )
        written_train = write_split(
            train_records,
            train_dir,
            args.prefix,
            args.class_prefix,
            1,
            move_files,
        )
        written_test = write_split(
            test_records,
            test_dir,
            args.prefix,
            args.class_prefix,
            len(train_records) + 1,
            move_files,
        )

    manifest = build_manifest(
        args,
        image_paths,
        valid_records,
        invalid_paths,
        duplicate_paths,
        train_records,
        test_records,
        written_train,
        written_test,
    )

    if not args.dry_run:
        manifest_path = output_root / args.manifest_name
        manifest_path.write_text(
            json.dumps(manifest, indent=2, ensure_ascii=False),
            encoding="utf-8",
        )

    print_summary(manifest, args.dry_run)


if __name__ == "__main__":
    main()
