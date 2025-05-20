import os
from PIL import Image
from torch.utils.data import Dataset, DataLoader
import torchvision.transforms as transforms
import time 
class TinyImageNetDataset(Dataset):
    def __init__(self, root_dir, split='train', transform=None):
        self.root_dir = root_dir
        self.split = split
        self.transform = transform
        self.samples = []  # list of (image_path, label)
        self.class_to_idx = {}

        if split == 'train':
            train_dir = os.path.join(root_dir, 'train')
            classes = sorted(os.listdir(train_dir))
            self.class_to_idx = {cls: idx for idx, cls in enumerate(classes)}
            for cls in classes:
                img_dir = os.path.join(train_dir, cls, 'images')
                for img_name in sorted(os.listdir(img_dir)):
                    self.samples.append((
                        os.path.join(img_dir, img_name),
                        self.class_to_idx[cls]
                    ))
        else:
            raise ValueError("Only 'train' split is supported in this example")

    def __len__(self):
        return len(self.samples)

    def __getitem__(self, idx):
        #t0 = time.time()
        img_path, label = self.samples[idx]
        image = Image.open(img_path).convert('RGB')
        #t1 = time.time()  # 디스크 I/O 이후 시점
        if self.transform:
            image = self.transform(image)
            
        #t2 = time.time()  # 전체 처리 완료 시점
        
        #print(f"[idx {idx}] Disk I/O: {t1 - t0:.6f}s, Transform: {t2 - t1:.6f}s, Total: {t2 - t0:.6f}s")
        
        return image, label

def get_dataloader(root_dir, batch_size=32, num_workers=4):
    # Preprocessing pipeline: decode, random crop, flip, to tensor, normalize
    transform = transforms.Compose([
        # 1) Decode is handled by PIL Image.open
        # 2) RandomResizedCrop: random crop and resize to 64x64
        transforms.RandomResizedCrop(64),
        # 3) RandomHorizontalFlip: random horizontal flip
        transforms.RandomHorizontalFlip(),
        # 4) ToTensor: convert to [0.0,1.0] tensor
        transforms.ToTensor(),
        # 5) Normalize with ImageNet mean and std
        transforms.Normalize(
            mean=[0.485, 0.456, 0.406],
            std =[0.229, 0.224, 0.225],
        ),
    ])
    dataset = TinyImageNetDataset(root_dir, split='train', transform=transform)
    return DataLoader(dataset, batch_size=batch_size, shuffle=True, num_workers=num_workers), len(dataset.class_to_idx)

