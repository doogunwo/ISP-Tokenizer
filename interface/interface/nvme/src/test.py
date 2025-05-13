import os

UTF8_BLOCK = 4096
TOTAL_SIZE = 128 * 1024  # 131072 bytes
INPUT = "wiki_128kb_utf8.txt"
OUTPUT_DIR = "utf8_blocks"

def write_utf8_blocks(input_path, block_size=UTF8_BLOCK, total_size=TOTAL_SIZE):
    with open(input_path, "rb") as f:
        raw = f.read(total_size)

    os.makedirs(OUTPUT_DIR, exist_ok=True)
    offset = 0
    part = 0

    while offset < len(raw):
        end = min(offset + block_size, len(raw))
        # UTF-8 경계 맞추기
        while end > offset:
            try:
                chunk = raw[offset:end].decode("utf-8")
                break
            except UnicodeDecodeError:
                end -= 1

        if end == offset:
            raise ValueError(f"[ERROR] UTF-8 경계 찾기 실패 at offset={offset}")

        with open(f"{OUTPUT_DIR}/block_{part:03d}.txt", "wb") as out:
            out.write(raw[offset:end])

        print(f"[INFO] 블록 {part:03d}: {end - offset} bytes 저장 (offset: {offset})")
        offset = end
        part += 1

    print(f"[DONE] {part} 개 블록 생성 완료")

write_utf8_blocks(INPUT)

