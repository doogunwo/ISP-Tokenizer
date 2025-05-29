import pyarrow.ipc as ipc
import os
import struct
import pyarrow as pa

filepath = os.path.expanduser("~/Desktop/tokenized_dataset/data-00000-of-00010.arrow")
with open(filepath, "rb") as f:
    reader = ipc.RecordBatchFileReader(f)

    print("📐 Schema:")
    print(reader.schema)

    print(f"📦 RecordBatch 수: {reader.num_record_batches}")
    for i in range(reader.num_record_batches):
        batch = reader.get_batch(i)
        print(f"  - Batch {i}: {batch.num_rows} rows, {batch.nbytes} bytes")
