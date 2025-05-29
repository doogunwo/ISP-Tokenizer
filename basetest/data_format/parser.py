import pyarrow.ipc as ipc
import pyarrow as pa
import os

def analyze_arrow_file(filepath):
    print(f"📂 분석 파일 경로: {filepath}")

    with open(filepath, "rb") as f:
        reader = ipc.RecordBatchFileReader(f)

        # 스키마 정보 출력
        schema = reader.schema
        print("\n📐 Schema:")
        print(schema)

        # RecordBatch 수
        num_batches = reader.num_record_batches
        print(f"\n🧱 RecordBatch 수: {num_batches}")

        # 배치 정보 출력
        total_rows = 0
        total_bytes = 0

        print("\n📦 각 RecordBatch 정보:")
        for i in range(num_batches):
            batch = reader.get_batch(i)
            rows = batch.num_rows
            size = batch.nbytes
            print(f"  - Batch {i}: {rows} rows, {size / 1024:.2f} KB")
            total_rows += rows
            total_bytes += size

        print(f"\n📈 전체 row 수: {total_rows}")
        print(f"📏 전체 데이터 크기 (압축 해제 기준): {total_bytes / (1024 * 1024):.2f} MB")

def print_block_offsets(filepath):
    with open(filepath, 'rb') as f:
        reader = ipc.RecordBatchFileReader(f)
        schema = reader.schema

        print("\n📐 Schema:")
        print(schema)

        print(f"\n🧱 RecordBatch 수: {reader.num_record_batches}")
        for i in range(reader.num_record_batches):
            batch = reader.get_batch(i)
            print(f"  - Batch {i}: rows={batch.num_rows}, size={batch.nbytes / 1024:.2f} KB")

        # 다음 정보는 내부 구조이며 공식 API로는 offset을 제공하지 않음

# 실행
if __name__ == "__main__":
    filepath = os.path.expanduser("~/Desktop/tokenized_dataset/standard.arrow")
    analyze_arrow_file(filepath)
    print_block_offsets(filepath)
