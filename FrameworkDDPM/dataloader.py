from pathlib import Path

from torch.utils.data import ConcatDataset, DataLoader
from torchvision import transforms
import numpy as np
import torchvision
import matplotlib.pyplot as plt


def normalize_dataset_names(dataset_names):
    """把单个目录名或多个目录名统一整理成列表。"""
    if dataset_names is None:
        return ["datasets-1"]
    if isinstance(dataset_names, (str, Path)):
        return [str(dataset_names)]
    return [str(dataset_name) for dataset_name in dataset_names]


def load_imagefolder_split(data_root, data_transform):
    """读取单个数据目录下的 train/test 子集，并做存在性检查。"""
    train_root = data_root / "train"
    test_root = data_root / "test"

    missing_roots = [
        path for path in (train_root, test_root) if not path.exists()
    ]
    if missing_roots:
        missing_text = ", ".join(str(path) for path in missing_roots)
        raise FileNotFoundError(
            f"数据目录不完整，缺少以下路径: {missing_text}"
        )

    train = torchvision.datasets.ImageFolder(
        root=str(train_root),
        transform=data_transform,
    )
    test = torchvision.datasets.ImageFolder(
        root=str(test_root),
        transform=data_transform,
    )
    return train, test


def load_transformed_dataset(
    img_size=256,
    batch_size=128,
    dataset_names=None,
    split="train",
    num_workers=0,
    pin_memory=False,
    shuffle=True,
    drop_last=False,
) -> DataLoader:
    """加载一个或多个数据目录，并返回指定 split 的 DataLoader。"""
    data_transforms = [
        transforms.Resize((img_size, img_size)),
        transforms.ToTensor(),  # Scales data into [0,1]
        transforms.Lambda(lambda t: (t * 2) - 1),  # Scale between [-1, 1]
    ]
    data_transform = transforms.Compose(data_transforms)

    dataset_roots = normalize_dataset_names(dataset_names)
    project_root = Path(__file__).resolve().parent
    datasets = []

    for dataset_name in dataset_roots:
        data_root = project_root / dataset_name
        train, test = load_imagefolder_split(data_root, data_transform)

        if split == "train":
            datasets.append(train)
        elif split == "test":
            datasets.append(test)
        elif split == "all":
            datasets.extend([train, test])
        else:
            raise ValueError(
                "split must be one of: 'train', 'test', 'all'"
            )

    dataset = datasets[0] if len(datasets) == 1 else ConcatDataset(datasets)

    return DataLoader(
        dataset,
        batch_size=batch_size,
        shuffle=shuffle,
        drop_last=drop_last,
        num_workers=num_workers,
        pin_memory=pin_memory,
    )


def show_tensor_image(image):
    # Reverse the data transformations
    reverse_transforms = transforms.Compose(
        [
            transforms.Lambda(lambda t: (t + 1) / 2),
            transforms.Lambda(lambda t: t.permute(1, 2, 0)),  # CHW to HWC
            transforms.Lambda(lambda t: t * 255.0),
            transforms.Lambda(lambda t: t.numpy().astype(np.uint8)),
            transforms.ToPILImage(),
        ]
    )

    # Take first image of batch
    if len(image.shape) == 4:
        image = image[0, :, :, :]
    plt.imshow(reverse_transforms(image))
