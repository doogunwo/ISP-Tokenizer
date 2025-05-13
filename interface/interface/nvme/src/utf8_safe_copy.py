import os

CHUNK_SIZE = 131072  # 128KB

def utf8_safe_slice(data, start, size):
    """
    주어진 바이트 시퀀스(data)의 start부터 최대 size까지 슬라이스하되,
    UTF-8 continuation byte가 끝에 포함되지 않도록 안전하게 자름.
    """
    end = min(start + size, len(data))
    while end > start and (data[end - 1] & 0b11000000) == 0b10000000:
        end -= 1
    return data[start:end]

def split_utf8_chunks(text_bytes):
    """
    고정 크기 기준으로 텍스트를 안전하게 UTF-8 경계에 맞춰 청크로 자름.
    """
    chunks = []
    i = 0
    while i < len(text_bytes):
        chunk = utf8_safe_slice(text_bytes, i, CHUNK_SIZE)
        if not chunk:
            # 이상하게 자를 게 없다면 그냥 강제로 넘어감 (무한 루프 방지)
            i += CHUNK_SIZE
            continue
        chunks.append(chunk)
        i += len(chunk)
    return chunks

def generate_large_utf8_file(input_path, output_filename, target_mb):
    """
    입력 UTF-8 텍스트 파일을 기반으로, target_mb 만큼의 유효 UTF-8 데이터를 반복하여 생성.
    """
    with open(input_path, 'rb') as f:
        raw_bytes = f.read()

    utf8_chunks = split_utf8_chunks(raw_bytes)
    print(f"[INFO] 유효한 UTF-8 청크 수: {len(utf8_chunks)}")

    target_bytes = target_mb * 1024 * 1024
    current_bytes = 0
    output_path = os.path.join("/mnt/nvme", output_filename)

    with open(output_path, 'wb') as out:
        i = 0
        while current_bytes < target_bytes:
            chunk = utf8_chunks[i % len(utf8_chunks)]
            out.write(chunk)
            current_bytes += len(chunk)
            i += 1

    actual_mb = os.path.getsize(output_path) / (1024 * 1024)
    print(f"[완료] {output_path} 생성됨 - 실제 크기: {actual_mb:.2f} MB")

if __name__ == "__main__":
    generate_large_utf8_file("wiki_english_only.txt", "wiki_150mb.txt", 150)

