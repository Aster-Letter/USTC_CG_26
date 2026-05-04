# Environment Setup & Parameter Reference

## Installation

The framework dependencies are listed in two `requirements` files. Choose the one that matches your hardware:

```bash
# With CUDA GPU (recommended)
pip install -r requirements.txt

# CPU-only (no NVIDIA GPU)
pip install -r requirements-cpu.txt
```

## Quick Start (Smoke Test)

Run a minimal training to verify the pipeline works:

```bash
# With CUDA GPU
python train_course_baseline.py --dataset_format pt \
    --data_path /path/to/dataset --out_dir runs/smoke_test \
    --max_steps 20 --batch_size 4 --max_items 4 \
    --latent_dim 256 --num_layers 4 --num_heads 4 \
    --view_layers 4 --view_num_heads 4 \
    --patch_size 8 --texture_patch_size 1 \
    --log_every 1 --vis_every 10 --save_every 500 \
    --workers 0 --device cuda

# CPU-only (slower)
python train_course_baseline.py --dataset_format pt \
    --data_path /path/to/dataset --out_dir runs/smoke_test \
    --max_steps 20 --batch_size 4 --max_items 4 \
    --latent_dim 256 --num_layers 4 --num_heads 4 \
    --view_layers 4 --view_num_heads 4 \
    --patch_size 8 --texture_patch_size 1 \
    --log_every 1 --vis_every 10 --save_every 500 \
    --workers 0 --device cpu
```

## Recommended Training Commands

### With DPT Decoder (Recommended)

Enabling the DPT decoder significantly improves rendering quality:

```bash
python train_course_baseline.py --dataset_format pt \
    --data_path /path/to/dataset --out_dir runs/experiment \
    --max_steps 30000 --batch_size 4 \
    --latent_dim 256 --num_layers 4 --num_heads 4 \
    --view_layers 6 --view_num_heads 4 \
    --patch_size 8 --texture_patch_size 1 \
    --use_dpt_decoder \
    --log_every 50 --vis_every 500 --save_every 5000 \
    --workers 0 --device cuda
```

> `--use_dpt_decoder` enables the DPT decoder head for better rendering detail. Requires `--view_layers >= 4` (6 recommended).

### Without DPT (Low VRAM)

If GPU memory is limited:

```bash
python train_course_baseline.py --dataset_format pt \
    --data_path /path/to/dataset --out_dir runs/experiment_no_dpt \
    --max_steps 30000 --batch_size 4 \
    --latent_dim 256 --num_layers 4 --num_heads 4 \
    --view_layers 4 --view_num_heads 4 \
    --patch_size 8 --texture_patch_size 1 \
    --log_every 50 --vis_every 500 --save_every 5000 \
    --workers 0 --device cuda
```

### Resume Training

Resume from the latest checkpoint:

```bash
python train_course_baseline.py --dataset_format pt \
    --data_path /path/to/dataset --out_dir runs/experiment \
    --resume_from runs/experiment/checkpoints/last.pt \
    --max_steps 30000 ...   # total target steps, NOT additional steps
```

## Formal Workflow for HW8

The homework requires a reportable PSNR result, so a complete run should include three stages: train, evaluate, and curate figures.

### Stage 1: Formal Training Run

Recommended command for the final result run:

```bash
python train_course_baseline.py --dataset_format pt \
    --data_path datasets/pt_dataset_teapot_v2 \
    --out_dir runs/hw8_formal_dpt \
    --max_steps 30000 --batch_size 4 \
    --latent_dim 256 --num_layers 4 --num_heads 4 \
    --view_layers 6 --view_num_heads 4 \
    --patch_size 8 --texture_patch_size 1 \
    --use_dpt_decoder \
    --log_every 50 --vis_every 500 --save_every 5000 \
    --workers 0 --device cuda
```

Artifacts produced by the trainer:

- `args.json`: frozen run configuration
- `metrics.jsonl`: one JSON record per training step, suitable for plotting loss curves
- `train_summary.json`: run summary and final checkpoint pointer
- `checkpoints/*.pt`: periodic and final checkpoints
- `vis/*.png`: side-by-side GT/pred previews

### Stage 2: Checkpoint Evaluation and PSNR

Use the new evaluation script on the final or best checkpoint:

```bash
python evaluate_course_baseline.py \
    --checkpoint runs/hw8_formal_dpt/checkpoints/final.pt \
    --out_dir runs/hw8_formal_dpt_eval \
    --device cuda --batch_size 1 --workers 0 --save_images 16
```

Evaluation outputs:

- `summary.json`: aggregated metrics over the evaluated set
- `per_sample_metrics.jsonl`: per-view PSNR / MSE records
- `vis/*.png`: saved GT-vs-pred comparison figures

The summary contains two PSNR variants:

- `mean_psnr_clamped`: PSNR after clamping predictions and GT to `[0, 1]`; this is the recommended report-facing metric.
- `mean_psnr_display`: PSNR after clamp + gamma conversion; useful as a supplementary perceptual metric.

### Stage 3: Report Figure Curation

For the report, collect at least the following evidence from the training and evaluation output folders:

1. Loss curve drawn from `metrics.jsonl`
2. Final PSNR from `summary.json`
3. Several `vis/*.png` comparisons covering both successful and weak views
4. The exact command line in `args.json` or your terminal log

### Stage 4: Draw the Loss Curve

The course baseline now includes a small plotting utility that reads `metrics.jsonl` and exports a PNG curve without requiring matplotlib:

```bash
python plot_training_metrics.py \
    --metrics runs/hw8_formal_dpt/metrics.jsonl \
    --out runs/hw8_formal_dpt/loss_curve.png \
    --title "HW8 Formal Training Loss"
```

Default plotted curves:

1. `total_loss`
2. `base_loss`

If you only want one metric in the figure:

```bash
python plot_training_metrics.py \
    --metrics runs/hw8_formal_dpt/metrics.jsonl \
    --out runs/hw8_formal_dpt/base_loss_curve.png \
    --fields base_loss
```

## Parameter Reference

All command-line arguments for `train_course_baseline.py`, grouped by category.

### Data

| Argument | Type | Default | Description |
|----------|------|---------|-------------|
| `--dataset_format` | `pt` / `h5` | **required** | Dataset format. Use `pt` for course assignments. |
| `--data_path` | str | **required** | PT dataset directory or H5 file path. |
| `--max_items` | int | None | Only load first N PT samples (for debugging). |
| `--image_size` | int | 64 | Render resolution (H5 format only). |

### Training Loop

| Argument | Type | Default | Description |
|----------|------|---------|-------------|
| `--max_steps` | int | 2000 | Total training steps (not "extra steps"). |
| `--batch_size` | int | 1 | Batch size per step. |
| `--grad_accum_steps` | int | 1 | Gradient accumulation steps. |
| `--lr` | float | 2e-4 | Learning rate. |
| `--weight_decay` | float | 1e-4 | Weight decay. |
| `--warmup_steps` | int | 100 | LR warmup steps. |
| `--grad_clip` | float | 1.0 | Gradient clipping threshold. |
| `--seed` | int | 42 | Random seed. |
| `--workers` | int | 0 | DataLoader worker processes (0 recommended on Windows). |
| `--resume_from` | str | None | Checkpoint path for resume. |

### Device & Precision

| Argument | Type | Default | Description |
|----------|------|---------|-------------|
| `--device` | `auto`/`cuda`/`cpu` | `auto` | Training device. |
| `--amp` | `auto`/`bf16`/`fp16`/`none` | `auto` | Mixed precision mode. Auto-enabled on CUDA. |

### Logging & Output

| Argument | Type | Default | Description |
|----------|------|---------|-------------|
| `--out_dir` | str | `.../runs/default` | Output directory. |
| `--log_every` | int | 20 | Print log every N steps. |
| `--vis_every` | int | 200 | Save GT/prediction visualization every N steps. |
| `--initial_vis_steps` | int | 0 | Additionally save previews for the first N steps. Set to `0` to disable warm-up previews. |
| `--save_every` | int | 500 | Save checkpoint every N steps. |

### Model Architecture

| Argument | Type | Default | Description |
|----------|------|---------|-------------|
| `--latent_dim` | int | 256 | Transformer hidden dimension. |
| `--num_layers` | int | 4 | Encoder (view-independent) layers. |
| `--num_heads` | int | 4 | Encoder attention heads. |
| `--view_layers` | int | 4 | Decoder (view-dependent) layers. |
| `--view_num_heads` | int | 4 | Decoder attention heads. |
| `--use_dpt_decoder` | flag | False | Enable DPT decoder head (recommended). |
| `--num_register_tokens` | int | 4 | Number of register tokens. |
| `--patch_size` | int | 8 | Image decoding patch size (resolution must be divisible). |
| `--texture_patch_size` | int | 8 | Triangle material patch size (use 1 for solid colors). |
| `--vertex_pe_num_freqs` | int | 6 | Vertex position encoding frequencies. |
| `--vn_pe_num_freqs` | int | 6 | Vertex normal encoding frequencies. |
| `--no_vn` | flag | False | Disable vertex normal encoder. |
| `--ffn_opt` | `checkpoint`/`none` | `checkpoint` | FFN optimization. `checkpoint` saves memory. |

### Loss

| Argument | Type | Default | Description |
|----------|------|---------|-------------|
| `--loss_type` | `log_l1`/`l1`/`mse` | `log_l1` | Loss function type. |
| `--use_lpips` | flag | False | Enable LPIPS perceptual loss (increases VRAM). |
| `--lpips_weight` | float | 0.05 | LPIPS loss weight. |

## Output Structure

During training, the following files are generated:

```
runs/experiment/
├── args.json                    # Saved run arguments
├── checkpoints/
│   ├── step_xxxxx.pt            # Periodic checkpoint
│   ├── last.pt                  # Latest checkpoint (updated every save_every)
│   └── final.pt                 # Final checkpoint at end of training
└── vis/
    └── step_xxxxx.png           # Side-by-side GT (left) and prediction (right)
```

## Memory Optimization

If VRAM is insufficient, apply these in order:

1. Reduce `--batch_size` (from 4 to 2 or 1)
2. Reduce `--latent_dim` (from 256 to 192 or 128)
3. Reduce `--num_layers` and `--view_layers`
4. Keep `--texture_patch_size 1` (for solid color materials)
5. Keep `--ffn_opt checkpoint`
6. Disable DPT (omit `--use_dpt_decoder`)
7. Disable LPIPS (omit `--use_lpips`)

## Notes

- Attention backend is fixed to `sdpa` — FlashAttention is not required.
- This directory is self-contained; it does not depend on the repo-root `renderformer/` package.
- Local compatibility shims for `einops` and `huggingface_hub` are included.
- `h5py` is included in both `requirements.txt` and `requirements-cpu.txt`.
- If the output is all black for the first few hundred steps, this is normal — wait for loss to drop.
- If the output remains all black indefinitely, restart training (random seed can affect convergence).
- `--max_steps` is the **total** target step count, not "extra steps to run".
