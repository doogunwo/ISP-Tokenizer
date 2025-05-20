import time
import torch
import torch.nn as nn
import torch.optim as optim
from dataloader import get_dataloader
from model import build_model

import psutil                    # CPU 사용률 측정
import pynvml                    # GPU 사용률 측정

# CPU 사용률 (%)
def get_cpu_usage():
    return psutil.cpu_percent(interval=None)

# GPU 사용률 (%)
def get_gpu_usage():
    handle = pynvml.nvmlDeviceGetHandleByIndex(0)
    util = pynvml.nvmlDeviceGetUtilizationRates(handle)
    return util.gpu

def main():
    # NVIDIA GPU 측정용 초기화
    pynvml.nvmlInit()

    # 1) DataLoader setup
    dataloader, num_classes = get_dataloader(
        root_dir='/mnt/nvme/tiny-imagenet-200/',
        batch_size=32,
        num_workers=16
    )

    # 2) Device setup
    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')

    # 3) Model, loss, optimizer
    model = build_model(num_classes=num_classes, pretrained=False)
    model = model.to(device)
    criterion = nn.CrossEntropyLoss()
    optimizer = optim.Adam(model.parameters(), lr=1e-3)

    # 4) Training loop with timing and utilization
    for epoch in range(1, 101):
        model.train()
        total_load_time = 0.0       # Data load + decode + transform
        total_copy_time = 0.0       # Host → GPU
        total_compute_time = 0.0    # Forward + backward + step
        total_loss = 0.0
        batch_count = 0

        cpu_usages = []
        gpu_usages = []
        gpu_idle_count = 0

        start_epoch = time.perf_counter()
        data_iter = iter(dataloader)

        while True:
            try:
                # 4.1 Data loading
                t0 = time.perf_counter()
                images, labels = next(data_iter)
                t1 = time.perf_counter()
                total_load_time += (t1 - t0)

                # 4.2 Copy to GPU
                t2 = time.perf_counter()
                images = images.to(device, non_blocking=True)
                labels = labels.to(device, non_blocking=True)
                t3 = time.perf_counter()
                total_copy_time += (t3 - t2)

                # 4.3 Compute
                optimizer.zero_grad()
                outputs = model(images)
                loss = criterion(outputs, labels)
                loss.backward()
                optimizer.step()
                t4 = time.perf_counter()
                total_compute_time += (t4 - t3)

                # 4.4 Utilization tracking
                cpu_usage = get_cpu_usage()
                gpu_usage = get_gpu_usage()

                cpu_usages.append(cpu_usage)
                gpu_usages.append(gpu_usage)
                if gpu_usage < 10:  # GPU 거의 놀고 있음
                    gpu_idle_count += 1

                total_loss += loss.item()
                batch_count += 1

            except StopIteration:
                break

        end_epoch = time.perf_counter()
        epoch_time = end_epoch - start_epoch
        avg_loss = total_loss / batch_count
        avg_cpu = sum(cpu_usages) / len(cpu_usages)
        avg_gpu = sum(gpu_usages) / len(gpu_usages)
        gpu_idle_ratio = gpu_idle_count / len(gpu_usages) * 100
        cpu_idle_ratio = 100 - avg_cpu

        print(f"Epoch {epoch:3d} | "
              f"Total: {epoch_time:.2f}s | "
              f"Load: {total_load_time:.2f}s | "
              f"Copy: {total_copy_time:.2f}s | "
              f"Compute: {total_compute_time:.2f}s | "
              f"Loss: {avg_loss:.4f} | "
              f"CPU: {avg_cpu:.1f}% used, {cpu_idle_ratio:.1f}% idle | "
              f"GPU: {avg_gpu:.1f}% used, {gpu_idle_ratio:.1f}% idle")

if __name__ == '__main__':
    main()
