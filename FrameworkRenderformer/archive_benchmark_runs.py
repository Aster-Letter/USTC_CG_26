from __future__ import annotations

import argparse
import csv
import json
import shutil
from pathlib import Path
from typing import Any


SMALL_ARTIFACTS = {
    "args.json": "configs",
    "train_summary.json": "summaries",
    "metrics.jsonl": "metrics",
}


def build_parser() -> argparse.ArgumentParser:
    script_dir = Path(__file__).resolve().parent
    repo_root = script_dir.parent
    parser = argparse.ArgumentParser(
        description="Archive lightweight benchmark evidence and optionally remove benchmark checkpoints.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument("--runs_dir", type=Path, default=script_dir / "runs")
    parser.add_argument(
        "--archive_dir",
        type=Path,
        default=repo_root / "docs-agent" / "hw8" / "benchmarks" / "20260505_speed_bench",
    )
    parser.add_argument("--pattern", default="bench_*", help="Run directory glob under runs_dir.")
    parser.add_argument(
        "--delete_checkpoints",
        action="store_true",
        help="Delete .pt checkpoint files inside matched benchmark run directories after archiving.",
    )
    return parser


def copy_if_exists(src: Path, dst: Path) -> bool:
    if not src.exists():
        return False
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)
    return True


def load_json(path: Path) -> dict[str, Any]:
    if not path.exists():
        return {}
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        return {}


def safe_delete_checkpoint(path: Path, run_dir: Path) -> int:
    resolved = path.resolve()
    resolved_run = run_dir.resolve()
    if resolved.suffix.lower() != ".pt":
        raise ValueError(f"Refusing to delete non-.pt file: {resolved}")
    if resolved_run not in resolved.parents:
        raise ValueError(f"Refusing to delete outside run dir: {resolved}")
    size = resolved.stat().st_size
    resolved.unlink()
    return size


def main() -> None:
    args = build_parser().parse_args()
    runs_dir = args.runs_dir.resolve()
    archive_dir = args.archive_dir.resolve()
    run_dirs = sorted(path for path in runs_dir.glob(args.pattern) if path.is_dir())

    archive_dir.mkdir(parents=True, exist_ok=True)
    manifest: list[dict[str, Any]] = []

    for run_dir in run_dirs:
        run_name = run_dir.name
        record: dict[str, Any] = {
            "run_name": run_name,
            "run_dir": str(run_dir),
            "copied_files": [],
            "deleted_checkpoints": [],
            "deleted_checkpoint_bytes": 0,
        }

        for artifact, folder in SMALL_ARTIFACTS.items():
            src = run_dir / artifact
            dst = archive_dir / folder / f"{run_name}_{artifact}"
            if copy_if_exists(src, dst):
                record["copied_files"].append(str(dst))

        vis_dir = run_dir / "vis"
        if vis_dir.exists():
            for src in sorted(path for path in vis_dir.rglob("*") if path.is_file()):
                dst = archive_dir / "vis" / run_name / src.relative_to(vis_dir)
                if copy_if_exists(src, dst):
                    record["copied_files"].append(str(dst))

        summary = load_json(run_dir / "train_summary.json")
        if summary:
            record["final_step"] = summary.get("final_step")
            elapsed = summary.get("elapsed_seconds")
            record["elapsed_seconds"] = elapsed
            if elapsed and summary.get("final_step"):
                record["seconds_per_step"] = float(elapsed) / int(summary["final_step"])

        if args.delete_checkpoints:
            for checkpoint in sorted(run_dir.rglob("*.pt")):
                deleted_bytes = safe_delete_checkpoint(checkpoint, run_dir)
                record["deleted_checkpoints"].append(str(checkpoint))
                record["deleted_checkpoint_bytes"] += deleted_bytes

        manifest.append(record)

    (archive_dir / "manifest.json").write_text(
        json.dumps(manifest, indent=2, ensure_ascii=False),
        encoding="utf-8",
    )

    with (archive_dir / "summary.csv").open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=[
                "run_name",
                "final_step",
                "elapsed_seconds",
                "seconds_per_step",
                "deleted_checkpoint_bytes",
            ],
        )
        writer.writeheader()
        for record in manifest:
            writer.writerow({key: record.get(key, "") for key in writer.fieldnames})

    readme_lines = [
        "# HW8 Speed Benchmark Archive",
        "",
        "This directory keeps lightweight evidence from short benchmark runs.",
        "Checkpoint `.pt` files are intentionally omitted because they are large and not useful for report analysis.",
        "",
        "| run | steps | seconds/step | deleted checkpoint bytes |",
        "| --- | ---: | ---: | ---: |",
    ]
    for record in manifest:
        sps = record.get("seconds_per_step")
        sps_text = f"{sps:.4f}" if isinstance(sps, float) else ""
        readme_lines.append(
            f"| `{record['run_name']}` | {record.get('final_step', '')} | {sps_text} | {record['deleted_checkpoint_bytes']} |"
        )
    (archive_dir / "README.md").write_text("\n".join(readme_lines) + "\n", encoding="utf-8")

    print(f"Archived {len(manifest)} benchmark runs to {archive_dir}")
    if args.delete_checkpoints:
        deleted_files = sum(len(record["deleted_checkpoints"]) for record in manifest)
        deleted_bytes = sum(record["deleted_checkpoint_bytes"] for record in manifest)
        print(f"Deleted {deleted_files} checkpoint files ({deleted_bytes / (1024 ** 2):.2f} MiB)")


if __name__ == "__main__":
    main()
