from datasets import load_from_disk
from transformers import (
    GPT2LMHeadModel,
    DataCollatorForLanguageModeling,
)
import torch
import time
from transformers import GPT2Tokenizer
from torch.utils.data import DataLoader
from torch.utils.data import SequentialSampler

# 🧩 read_bytes 측정 함수
def get_actual_disk_read_bytes():
    with open("/proc/self/io", "r") as f:
        for line in f:
            if line.startswith("read_bytes:"):
                return int(line.strip().split(":")[1])

# tokenizer
tokenizer = GPT2Tokenizer.from_pretrained("gpt2")
tokenizer.pad_token = tokenizer.eos_token

# device
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
print("✅ GPU available:", torch.cuda.is_available())
if torch.cuda.is_available():
    print("🖥️  GPU name:", torch.cuda.get_device_name(0))

# dataset
dataset = load_from_disk("/home/doogunwo/Desktop/tokenized_dataset")
dataset.set_format(type="torch", columns=["input_ids", "attention_mask"])
print(f"✅ Dataset loaded: {len(dataset)} samples")

# model
model = GPT2LMHeadModel.from_pretrained("gpt2").to(device)

# collator
data_collator = DataCollatorForLanguageModeling(tokenizer=tokenizer, mlm=False)

# metrics
io_times = []
collate_times = []
copy_times = []
compute_times = []
read_bytes_deltas = []

# dataloader
class TimedDataLoader(DataLoader):
    def __iter__(self):
        prev_read_bytes = get_actual_disk_read_bytes()

        for batch_indices in self.batch_sampler:
            # 1. 디스크 읽기 전 I/O 측정
            t0 = time.perf_counter()
            batch_raw = [self.dataset[i] for i in batch_indices]
            t1 = time.perf_counter()
            io_times.append(t1 - t0)

            # 실제 read bytes 측정
            current_read_bytes = get_actual_disk_read_bytes()
            delta_bytes = current_read_bytes - prev_read_bytes
            read_bytes_deltas.append(delta_bytes)
            prev_read_bytes = current_read_bytes

            # 2. Collate 측정
            t2 = time.perf_counter()
            batch = self.collate_fn(batch_raw)
            t3 = time.perf_counter()
            collate_times.append(t3 - t2)

            yield batch

sampler = torch.utils.data.RandomSampler(dataset)
#sampler = SequentialSampler(dataset)
batch_sampler = torch.utils.data.BatchSampler(sampler, batch_size=32, drop_last=True)
dataloader = TimedDataLoader(dataset, batch_sampler=batch_sampler, collate_fn=data_collator)

# optimizer
optimizer = torch.optim.AdamW(model.parameters(), lr=5e-5)

print("🚀 Training start")
model.train()
for step, batch in enumerate(dataloader):
    # 3. GPU로 복사 시간 측정
    t4 = time.perf_counter()
    batch = {k: v.to(device, non_blocking=True) for k, v in batch.items()}
    t5 = time.perf_counter()
    copy_times.append(t5 - t4)

    # 4. 연산 시간 측정
    t6 = time.perf_counter()
    outputs = model(**batch)
    loss = outputs.loss
    loss.backward()
    optimizer.step()
    optimizer.zero_grad()
    t7 = time.perf_counter()
    compute_times.append(t7 - t6)

    if step % 10 == 0:
        logical_bytes = batch["input_ids"].numel() * batch["input_ids"].element_size()
        physical_kb = read_bytes_deltas[-1] / 1024
        logical_kb = logical_bytes / 1024
        print(f"[Step {step:3d}] IO: {io_times[-1]:.4f}s | Collate: {collate_times[-1]:.4f}s | Copy: {copy_times[-1]:.4f}s | Compute: {compute_times[-1]:.4f}s | Loss: {loss.item():.4f}")
        print(f"          ↪ Requested: {logical_kb:.2f} KB | Actual Disk Read: {physical_kb:.2f} KB")

    if step == 200:
        break

# 요약 출력
print("\n📊 Timing Summary (200 steps):")
print(f"  Avg IO Time per Batch      : {sum(io_times) / len(io_times):.4f} sec")
print(f"  Avg Collate Time per Batch : {sum(collate_times) / len(collate_times):.4f} sec")
print(f"  Avg Copy Time per Batch    : {sum(copy_times) / len(copy_times):.4f} sec")
print(f"  Avg Compute Time per Batch : {sum(compute_times) / len(compute_times):.4f} sec")
print(f"  Avg Actual Disk Read       : {sum(read_bytes_deltas) / len(read_bytes_deltas) / 1024:.2f} KB")
