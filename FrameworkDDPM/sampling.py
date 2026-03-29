import json
from pathlib import Path

import cv2 as cv
import matplotlib.pyplot as plt
import numpy as np
import torch

from forward_noising import (
    betas,
    forward_diffusion_sample,
    get_index_from_list,
    posterior_variance,
    sqrt_one_minus_alphas_cumprod,
    sqrt_recip_alphas,
)
from rect_mask_tool import (
    MASK_PRESETS,
    create_mask,
    create_preview,
    load_image as load_mask_source_image,
    resolve_preset_rectangle,
)
from unet import SimpleUnet


PROJECT_ROOT = Path(__file__).resolve().parent
WORKSPACE_ROOT = PROJECT_ROOT.parents[2]
DEFAULT_CHECKPOINT = PROJECT_ROOT / "ddpm_mse_epochs_5000.pth"
MODEL_CHECKPOINT = PROJECT_ROOT / "model" / "ddpm_heart_colorful.pth"
FALLBACK_CHECKPOINT = WORKSPACE_ROOT / "temp" / "ddpm_mse_epochs_5000.pth"
DEFAULT_INPAINTING_IMAGE = PROJECT_ROOT / "inpainting_input.png"
LEGACY_INPAINTING_MASK = PROJECT_ROOT / "inpainting_mask.png"
DEFAULT_MASK_DIR = PROJECT_ROOT / "inpainting_masks"
GENERATED_MASK_DIR = DEFAULT_MASK_DIR / "generated"
DEFAULT_MASK_PRESET = "center-medium"
DEFAULT_BATCH_MASK_PRESETS = (
    "center-small",
    "center-medium",
    "center-wide",
    "center-tall",
    "left-band",
    "right-band",
    "top-band",
    "bottom-band",
    "top-left",
    "top-right",
    "bottom-left",
    "bottom-right",
)
DEFAULT_INPAINTING_BATCH_DIR = PROJECT_ROOT / "inpainting_batch"
DEFAULT_BATCH_MANIFEST_NAME = "batch_manifest.json"
MASK_IMAGE_SUFFIXES = (".png", ".jpg", ".jpeg", ".bmp", ".webp")
IMAGE_SUFFIXES = MASK_IMAGE_SUFFIXES
DEFAULT_IMG_SIZE = 256
DEFAULT_TIMESTEPS = 300
DEFAULT_SAMPLE_SEED = 3407
DEFAULT_PROGRESS_PATH = PROJECT_ROOT / "sampling_progress.png"


def select_device():
    """优先选择 CUDA；若不可用则回退到 CPU。"""
    if torch.cuda.is_available():
        return torch.device("cuda")
    return torch.device("cpu")


def load_trained_model(device, checkpoint_path=DEFAULT_CHECKPOINT):
    """加载训练好的 U-Net 权重，并切到 eval 模式。"""
    checkpoint_path = Path(checkpoint_path)
    if not checkpoint_path.exists():
        if checkpoint_path == DEFAULT_CHECKPOINT:
            for candidate in (MODEL_CHECKPOINT, FALLBACK_CHECKPOINT):
                if candidate.exists():
                    checkpoint_path = candidate
                    break
        else:
            raise FileNotFoundError(
                f"未找到模型权重: {checkpoint_path}。请先运行 training_model.py 完成训练。"
            )

    if not checkpoint_path.exists():
        raise FileNotFoundError(
            "未找到默认模型权重。已检查路径: "
            f"{DEFAULT_CHECKPOINT}, {MODEL_CHECKPOINT}, {FALLBACK_CHECKPOINT}"
        )

    model = SimpleUnet().to(device)
    state_dict = torch.load(checkpoint_path, map_location=device)
    model.load_state_dict(state_dict)
    model.eval()
    return model


def make_generator(device, seed=None):
    """为采样过程创建可选的随机数生成器，便于复现实验结果。"""
    if seed is None:
        return None

    generator = torch.Generator(device=device.type)
    generator.manual_seed(seed)
    return generator


def tensor_to_uint8_image(image):
    """把 [-1, 1] 范围的图像张量转换成可保存的 uint8 RGB 图像。"""
    image = image.detach().cpu().clamp(-1, 1)
    if image.ndim == 4:
        image = image[0]
    image = ((image.permute(1, 2, 0) + 1.0) / 2.0) * 255.0
    return image.numpy().astype(np.uint8)


def save_rgb_image(image, output_path):
    """把 RGB 的 uint8 图像保存为磁盘文件。"""
    output_path = Path(output_path)
    cv.imwrite(
        str(output_path),
        cv.cvtColor(image, cv.COLOR_RGB2BGR),
    )
    return output_path


def save_sampling_overview(frames, output_path):
    """把若干采样中间帧拼成一张总览图，替代阻塞式弹窗。"""
    if not frames:
        return None

    figure, axes = plt.subplots(1, len(frames), figsize=(4 * len(frames), 4))
    if len(frames) == 1:
        axes = [axes]

    for axis, (timestep, frame) in zip(axes, frames):
        axis.imshow(frame)
        axis.set_title(f"t = {timestep}")
        axis.axis("off")

    figure.tight_layout()
    output_path = Path(output_path)
    figure.savefig(output_path, dpi=150, bbox_inches="tight")
    plt.close(figure)
    return output_path


def save_image_preview(image, output_path, title):
    """保存单张预览图，避免脚本执行时弹出阻塞窗口。"""
    figure, axis = plt.subplots(figsize=(4, 4))
    axis.imshow(image)
    axis.set_title(title)
    axis.axis("off")
    figure.tight_layout()
    output_path = Path(output_path)
    figure.savefig(output_path, dpi=150, bbox_inches="tight")
    plt.close(figure)
    return output_path


def is_supported_image_file(path, suffixes):
    return path.is_file() and path.suffix.lower() in suffixes


def is_mask_file(path):
    return is_supported_image_file(
        path,
        MASK_IMAGE_SUFFIXES,
    ) and not path.stem.endswith("_preview")


def sanitize_name(text):
    sanitized = str(text).replace("\\", "_").replace("/", "_")
    sanitized = sanitized.replace(" ", "-").replace(":", "-")
    return sanitized


def list_available_masks(mask_dir=DEFAULT_MASK_DIR):
    mask_dir = Path(mask_dir)
    mask_files = []
    if mask_dir.exists():
        mask_files = [
            str(path.relative_to(mask_dir))
            for path in list_mask_files(mask_dir)
        ]

    return {
        "mask_dir": mask_dir,
        "mask_files": mask_files,
        "mask_presets": sorted(MASK_PRESETS),
        "default_batch_mask_presets": list(DEFAULT_BATCH_MASK_PRESETS),
    }


def resolve_inpainting_image_path(image_path=None):
    return Path(image_path) if image_path else DEFAULT_INPAINTING_IMAGE


def list_mask_files(mask_dir):
    mask_dir = Path(mask_dir)
    if not mask_dir.exists():
        return []

    return [
        path
        for path in sorted(mask_dir.rglob("*"))
        if is_mask_file(path)
    ]


def resolve_named_mask_path(mask_dir, mask_name):
    mask_dir = Path(mask_dir)
    direct_path = mask_dir / mask_name
    if direct_path.exists():
        return direct_path

    mask_stem = Path(mask_name).stem if Path(mask_name).suffix else mask_name
    for suffix in MASK_IMAGE_SUFFIXES:
        candidate = mask_dir / f"{mask_stem}{suffix}"
        if candidate.exists():
            return candidate

    for candidate in list_mask_files(mask_dir):
        if candidate.stem == mask_stem:
            return candidate

    raise FileNotFoundError(
        f"Mask '{mask_name}' was not found under: {mask_dir}"
    )


def generate_preset_mask_assets(
    image_path,
    mask_preset,
    mask_output_dir=GENERATED_MASK_DIR,
):
    image_path, image_bgr = load_mask_source_image(image_path)
    rectangle = resolve_preset_rectangle(
        mask_preset,
        image_bgr.shape[1],
        image_bgr.shape[0],
    )
    mask = create_mask(image_bgr.shape, rectangle)
    preview = create_preview(image_bgr, rectangle)

    preset_slug = mask_preset.replace(" ", "-")
    mask_output_dir = Path(mask_output_dir)
    mask_output_dir.mkdir(parents=True, exist_ok=True)

    mask_path = mask_output_dir / f"{image_path.stem}_{preset_slug}_mask.png"
    preview_path = (
        mask_output_dir / f"{image_path.stem}_{preset_slug}_preview.png"
    )
    cv.imwrite(str(mask_path), mask)
    cv.imwrite(str(preview_path), preview)

    return {
        "mask_path": mask_path,
        "mask_preview_path": preview_path,
        "mask_origin": "generated-preset",
        "mask_preset": mask_preset,
        "rectangle": rectangle,
    }


def resolve_inpainting_mask(
    image_path,
    mask_path=None,
    mask_name="",
    mask_dir=DEFAULT_MASK_DIR,
    mask_preset="",
):
    if mask_path:
        resolved_mask_path = Path(mask_path)
        if not resolved_mask_path.exists():
            raise FileNotFoundError(
                f"未找到指定 mask: {resolved_mask_path}"
            )
        return {
            "mask_path": resolved_mask_path,
            "mask_preview_path": None,
            "mask_origin": "explicit-path",
            "mask_preset": None,
        }

    if mask_name:
        resolved_mask_path = resolve_named_mask_path(mask_dir, mask_name)
        return {
            "mask_path": resolved_mask_path,
            "mask_preview_path": None,
            "mask_origin": "mask-dir",
            "mask_preset": None,
        }

    if mask_preset:
        return generate_preset_mask_assets(image_path, mask_preset)

    if LEGACY_INPAINTING_MASK.exists():
        return {
            "mask_path": LEGACY_INPAINTING_MASK,
            "mask_preview_path": None,
            "mask_origin": "legacy-default",
            "mask_preset": None,
        }

    mask_dir = Path(mask_dir)
    if mask_dir.exists():
        for default_name in ("default", DEFAULT_MASK_PRESET):
            try:
                resolved_mask_path = resolve_named_mask_path(
                    mask_dir,
                    default_name,
                )
                return {
                    "mask_path": resolved_mask_path,
                    "mask_preview_path": None,
                    "mask_origin": "mask-dir-default",
                    "mask_preset": None,
                }
            except FileNotFoundError:
                continue

        available_mask_files = list_mask_files(mask_dir)
        if len(available_mask_files) == 1:
            return {
                "mask_path": available_mask_files[0],
                "mask_preview_path": None,
                "mask_origin": "mask-dir-single",
                "mask_preset": None,
            }

    preset_name = mask_preset or DEFAULT_MASK_PRESET
    return generate_preset_mask_assets(image_path, preset_name)


def collect_inpainting_image_paths(
    image_path=None,
    image_dir=None,
    limit_images=0,
):
    if image_path and image_dir:
        raise ValueError("Use either image_path or image_dir, not both.")

    if image_dir:
        image_root = Path(image_dir)
        if not image_root.exists():
            raise FileNotFoundError(f"Image directory not found: {image_root}")

        image_paths = [
            path
            for path in sorted(image_root.rglob("*"))
            if is_supported_image_file(path, IMAGE_SUFFIXES)
        ]
        if not image_paths:
            raise FileNotFoundError(
                f"No supported images found under: {image_root}"
            )
    else:
        resolved_image_path = resolve_inpainting_image_path(image_path)
        if not resolved_image_path.exists():
            raise FileNotFoundError(
                f"Inpainting input image not found: {resolved_image_path}"
            )
        image_paths = [resolved_image_path]

    if limit_images > 0:
        image_paths = image_paths[:limit_images]
    return image_paths


def collect_batch_mask_specs(
    image_path,
    mask_dir=DEFAULT_MASK_DIR,
    mask_names=None,
    mask_presets=None,
    include_mask_dir_masks=False,
):
    specs = []
    seen_labels = set()
    mask_dir = Path(mask_dir)

    if include_mask_dir_masks:
        for mask_path in list_mask_files(mask_dir):
            relative_name = str(mask_path.relative_to(mask_dir))
            label = (
                f"mask-{sanitize_name(Path(relative_name).with_suffix(''))}"
            )
            if label in seen_labels:
                continue
            seen_labels.add(label)
            specs.append(
                {
                    "label": label,
                    "mask_name": relative_name,
                    "mask_origin": "mask-dir",
                }
            )

    for mask_name in mask_names or []:
        label = f"mask-{sanitize_name(Path(mask_name).with_suffix(''))}"
        if label in seen_labels:
            continue
        seen_labels.add(label)
        specs.append(
            {
                "label": label,
                "mask_name": mask_name,
                "mask_origin": "mask-dir",
            }
        )

    effective_presets = (
        list(mask_presets)
        if mask_presets is not None
        else list(DEFAULT_BATCH_MASK_PRESETS)
    )
    for mask_preset in effective_presets:
        if mask_preset not in MASK_PRESETS:
            raise ValueError(f"Unknown mask preset: {mask_preset}")
        label = f"preset-{sanitize_name(mask_preset)}"
        if label in seen_labels:
            continue
        seen_labels.add(label)
        specs.append(
            {
                "label": label,
                "mask_preset": mask_preset,
                "mask_origin": "generated-preset",
            }
        )

    if not specs:
        raise ValueError("No batch mask strategy was produced.")

    return specs


def image_batch_label(image_path, image_dir=None):
    image_path = Path(image_path)
    if image_dir:
        image_dir = Path(image_dir)
        relative_path = image_path.relative_to(image_dir)
        return sanitize_name(relative_path.with_suffix(""))
    return sanitize_name(image_path.stem)


def serialize_result_paths(result):
    serialized = {}
    for key, value in result.items():
        serialized[key] = str(value) if isinstance(value, Path) else value
    return serialized


def run_inpainting_batch(
    checkpoint_path=DEFAULT_CHECKPOINT,
    image_path=None,
    image_dir=None,
    mask_dir=DEFAULT_MASK_DIR,
    mask_names=None,
    mask_presets=None,
    include_mask_dir_masks=False,
    limit_images=0,
    output_dir=DEFAULT_INPAINTING_BATCH_DIR,
    manifest_name=DEFAULT_BATCH_MANIFEST_NAME,
):
    device = select_device()
    model = load_trained_model(device, checkpoint_path=checkpoint_path)
    image_paths = collect_inpainting_image_paths(
        image_path=image_path,
        image_dir=image_dir,
        limit_images=limit_images,
    )

    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    results = []

    for current_image_path in image_paths:
        current_image_path = Path(current_image_path)
        current_img = load_inpainting_image_tensor(device, current_image_path)
        batch_label = image_batch_label(
            current_image_path,
            image_dir=image_dir,
        )
        current_output_dir = output_dir / batch_label
        current_output_dir.mkdir(parents=True, exist_ok=True)

        mask_specs = collect_batch_mask_specs(
            current_image_path,
            mask_dir=mask_dir,
            mask_names=mask_names,
            mask_presets=mask_presets,
            include_mask_dir_masks=include_mask_dir_masks,
        )

        for mask_spec in mask_specs:
            mask_info = resolve_inpainting_mask(
                current_image_path,
                mask_name=mask_spec.get("mask_name", ""),
                mask_dir=mask_dir,
                mask_preset=mask_spec.get("mask_preset", ""),
            )
            current_mask = load_inpainting_mask_tensor(
                device,
                mask_info["mask_path"],
            )
            inpainted_img = inpaint(model, device, current_img, current_mask)
            inpainted_img_uint8 = tensor_to_uint8_image(inpainted_img)

            output_path = save_rgb_image(
                inpainted_img_uint8,
                current_output_dir / f"{mask_spec['label']}_inpainted.png",
            )
            preview_path = save_image_preview(
                inpainted_img_uint8,
                current_output_dir / f"{mask_spec['label']}_preview.png",
                f"{current_image_path.stem} | {mask_spec['label']}",
            )

            results.append(
                {
                    "input_path": current_image_path,
                    "mask_path": mask_info["mask_path"],
                    "mask_preview_path": mask_info["mask_preview_path"],
                    "mask_origin": mask_info["mask_origin"],
                    "mask_preset": mask_info["mask_preset"],
                    "output_path": output_path,
                    "preview_path": preview_path,
                    "batch_label": batch_label,
                    "mask_label": mask_spec["label"],
                }
            )

    manifest = {
        "checkpoint_path": str(checkpoint_path),
        "image_path": str(image_path) if image_path else "",
        "image_dir": str(image_dir) if image_dir else "",
        "mask_dir": str(mask_dir),
        "image_count": len(image_paths),
        "result_count": len(results),
        "results": [serialize_result_paths(result) for result in results],
    }
    manifest_path = output_dir / manifest_name
    manifest_path.write_text(
        json.dumps(manifest, indent=2, ensure_ascii=False),
        encoding="utf-8",
    )
    manifest["manifest_path"] = str(manifest_path)
    return manifest


def test_image_inpainting_batch(
    checkpoint_path=DEFAULT_CHECKPOINT,
    image_path=None,
    image_dir=None,
    mask_dir=DEFAULT_MASK_DIR,
    mask_names=None,
    mask_presets=None,
    include_mask_dir_masks=False,
    limit_images=0,
    output_dir=DEFAULT_INPAINTING_BATCH_DIR,
    manifest_name=DEFAULT_BATCH_MANIFEST_NAME,
):
    return run_inpainting_batch(
        checkpoint_path=checkpoint_path,
        image_path=image_path,
        image_dir=image_dir,
        mask_dir=mask_dir,
        mask_names=mask_names,
        mask_presets=mask_presets,
        include_mask_dir_masks=include_mask_dir_masks,
        limit_images=limit_images,
        output_dir=output_dir,
        manifest_name=manifest_name,
    )


@torch.no_grad()
def sample_timestep(model, x, t, generator=None):
    """执行一次 DDPM 逆扩散单步采样。

    输入：
    - x: 当前时间步 t 的带噪图像 x_t
    - t: 当前时间步，形状通常为 (batch_size,)

    输出：
    - 若 t > 0，返回加入随机项后的 x_(t-1)
    - 若 t == 0，返回最终去噪均值，不再额外注入噪声
    """
    predicted_noise = model(x, t)

    beta_t = get_index_from_list(betas, t, x.shape)
    sqrt_one_minus_alphas_cumprod_t = get_index_from_list(
        sqrt_one_minus_alphas_cumprod, t, x.shape
    )
    sqrt_recip_alphas_t = get_index_from_list(sqrt_recip_alphas, t, x.shape)
    posterior_variance_t = get_index_from_list(posterior_variance, t, x.shape)

    # DDPM 逆过程均值：先从 x_t 中减去模型预测的噪声，再按 alpha_t 缩放。
    model_mean = sqrt_recip_alphas_t * (
        x - beta_t * predicted_noise / sqrt_one_minus_alphas_cumprod_t
    )

    if torch.all(t == 0):
        return model_mean

    noise = torch.randn(
        x.shape,
        device=x.device,
        dtype=x.dtype,
        generator=generator,
    )
    return model_mean + torch.sqrt(posterior_variance_t) * noise


@torch.no_grad()
def sample_plot_image(
    model,
    device,
    img_size=DEFAULT_IMG_SIZE,
    T=DEFAULT_TIMESTEPS,
    show_every=50,
    seed=DEFAULT_SAMPLE_SEED,
    progress_path=DEFAULT_PROGRESS_PATH,
):
    """从纯高斯噪声开始，执行完整逆扩散并返回最终图像张量。

    这里额外支持 seed，是为了让人工测试时更容易复现“同一份权重得到什么结果”。
    """
    generator = make_generator(device, seed)
    img = torch.randn(
        (1, 3, img_size, img_size),
        device=device,
        generator=generator,
    )
    frames = []

    for timestep in reversed(range(T)):
        t = torch.full(
            (img.shape[0],),
            timestep,
            device=device,
            dtype=torch.long,
        )
        img = sample_timestep(model, img, t, generator=generator)

        should_show = (
            show_every is not None
            and (
                timestep % show_every == 0
                or timestep == T - 1
                or timestep == 0
            )
        )
        if should_show:
            frames.append((timestep, tensor_to_uint8_image(img)))

    progress_output = None
    if progress_path is not None:
        progress_output = save_sampling_overview(frames, progress_path)

    return img, progress_output


def test_image_generation(checkpoint_path=DEFAULT_CHECKPOINT):
    """加载训练权重，执行整图采样，并保存最终结果。"""
    device = select_device()
    model = load_trained_model(device, checkpoint_path=checkpoint_path)

    generated_img, progress_output = sample_plot_image(model, device)
    generated_img_uint8 = tensor_to_uint8_image(generated_img)

    output_path = save_rgb_image(
        generated_img_uint8,
        PROJECT_ROOT / "generated_image.png",
    )
    preview_output = save_image_preview(
        generated_img_uint8,
        PROJECT_ROOT / "generated_image_preview.png",
        "Generated Image",
    )

    return {
        "image_path": output_path,
        "preview_path": preview_output,
        "progress_path": progress_output,
    }


def load_inpainting_image_tensor(
    device,
    image_path,
    target_size=DEFAULT_IMG_SIZE,
):
    image_path = Path(image_path)
    image = cv.imread(str(image_path))
    if image is None:
        raise FileNotFoundError(f"未找到待补图像: {image_path}")

    image = cv.resize(
        image,
        (target_size, target_size),
        interpolation=cv.INTER_AREA,
    )
    image = cv.cvtColor(image, cv.COLOR_BGR2RGB)
    image_tensor = torch.from_numpy(image).float() / 255.0
    image_tensor = image_tensor.permute(2, 0, 1).unsqueeze(0)
    image_tensor = image_tensor * 2.0 - 1.0
    return image_tensor.to(device)


def load_inpainting_mask_tensor(
    device,
    mask_path,
    target_size=DEFAULT_IMG_SIZE,
):
    mask_path = Path(mask_path)
    mask = cv.imread(str(mask_path), cv.IMREAD_GRAYSCALE)
    if mask is None:
        raise FileNotFoundError(f"未找到补全 mask: {mask_path}")

    mask = cv.resize(
        mask,
        (target_size, target_size),
        interpolation=cv.INTER_NEAREST,
    )
    mask_tensor = torch.from_numpy(mask).float() / 255.0
    mask_tensor = (mask_tensor > 0.5).float().unsqueeze(0).unsqueeze(0)
    mask_tensor = mask_tensor.expand(-1, 3, -1, -1)
    return mask_tensor.to(device)


def load_inpainting_inputs(
    device,
    image_path,
    mask_path,
    target_size=DEFAULT_IMG_SIZE,
):
    """读取待补图像和 mask，并转换成与训练一致的张量格式。

    这里主动把输入 resize 到训练分辨率，避免用户直接放入任意大小图片后，
    因尺寸不匹配导致网络结构或补全过程出现不稳定现象。
    """
    image_tensor = load_inpainting_image_tensor(
        device,
        image_path,
        target_size=target_size,
    )
    mask_tensor = load_inpainting_mask_tensor(
        device,
        mask_path,
        target_size=target_size,
    )
    return image_tensor, mask_tensor


@torch.no_grad()
def inpaint(model, device, img, mask, t_max=50, seed=DEFAULT_SAMPLE_SEED):
    """按 RePaint 风格执行图像补全。

    mask 约定：
    - 1 表示该像素来自已知区域，应尽量保持原图信息
    - 0 表示该像素属于待补区域，应由扩散模型逐步生成
    """
    img = img.to(device)
    mask = mask.to(device)
    generator = make_generator(device, seed)

    start_t = torch.full(
        (img.shape[0],),
        t_max - 1,
        device=device,
        dtype=torch.long,
    )
    x_t, _ = forward_diffusion_sample(img, start_t, device)

    for timestep in range(t_max - 1, -1, -1):
        t = torch.full(
            (img.shape[0],),
            timestep,
            device=device,
            dtype=torch.long,
        )
        predicted_unknown = sample_timestep(model, x_t, t, generator=generator)

        if timestep > 0:
            previous_t = torch.full(
                (img.shape[0],),
                timestep - 1,
                device=device,
                dtype=torch.long,
            )
            known_region, _ = forward_diffusion_sample(img, previous_t, device)
        else:
            known_region = img

        x_t = mask * known_region + (1.0 - mask) * predicted_unknown

    return x_t


def test_image_inpainting(
    checkpoint_path=DEFAULT_CHECKPOINT,
    image_path=None,
    mask_path=None,
    mask_name="",
    mask_dir=DEFAULT_MASK_DIR,
    mask_preset="",
):
    """读取输入图像与 mask，执行补全，并保存结果。"""
    device = select_device()
    model = load_trained_model(device, checkpoint_path=checkpoint_path)

    resolved_image_path = resolve_inpainting_image_path(image_path)
    mask_info = resolve_inpainting_mask(
        resolved_image_path,
        mask_path=mask_path,
        mask_name=mask_name,
        mask_dir=mask_dir,
        mask_preset=mask_preset,
    )
    img, mask = load_inpainting_inputs(
        device,
        resolved_image_path,
        mask_info["mask_path"],
    )

    inpainted_img = inpaint(model, device, img, mask)
    inpainted_img_uint8 = tensor_to_uint8_image(inpainted_img)

    output_path = save_rgb_image(
        inpainted_img_uint8,
        PROJECT_ROOT / "inpainted_image.png",
    )
    preview_output = save_image_preview(
        inpainted_img_uint8,
        PROJECT_ROOT / "inpainted_image_preview.png",
        "Inpainted Image",
    )

    return {
        "image_path": output_path,
        "preview_path": preview_output,
        "input_path": resolved_image_path,
        "mask_path": mask_info["mask_path"],
        "mask_preview_path": mask_info["mask_preview_path"],
        "mask_origin": mask_info["mask_origin"],
        "mask_preset": mask_info["mask_preset"],
    }


if __name__ == "__main__":
    test_image_generation()

    image_path = DEFAULT_INPAINTING_IMAGE
    if image_path.exists():
        test_image_inpainting(image_path=image_path)
