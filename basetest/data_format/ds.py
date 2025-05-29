import os
from datasets import load_from_disk
import pyarrow as pa
import pyarrow.ipc as ipc

# ✔️ Hugging Face 데이터셋 로드
hf_dataset_path = os.path.expanduser("~/Desktop/dataset")
dataset = load_from_disk(hf_dataset_path)

# ✔️ 일부만 추출 (선택적)
subset = dataset.select(range(1000))

# ConcatenationTable → pyarrow.Table로 변환
arrow_table = pa.Table.from_batches(subset.data.to_batches())

# 📤 표준 Arrow IPC 포맷으로 저장
output_path = os.path.expanduser("~/Desktop/standard.arrow")
with open(output_path, "wb") as f:
    writer = ipc.RecordBatchFileWriter(f, arrow_table.schema)
    writer.write_table(arrow_table)
    writer.close()

print(f"✅ 표준 Arrow IPC 파일로 저장 완료: {output_path}")
