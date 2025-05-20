from torch.utils.data import DataLoader
import time

dataloader = DataLoader(tokenized_dataset, batch_size=64, shuffle=True)

for step, batch in enumerate(dataloader):
    t0 = time.perf_counter()
    inputs = {k: v.cuda() for k, v in batch.items()}
    t1 = time.perf_counter()
    outputs = model(**inputs)
    loss = outputs.loss
    t2 = time.perf_counter()
    print(f"[Step {step}] Load+Copy: {t1 - t0:.3f}s | Compute: {t2 - t1:.3f}s")
    if step > 20:
        break
