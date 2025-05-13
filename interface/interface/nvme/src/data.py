import os
import shutil

SRC_FILE = "wiki_corpus.txt"
DST_FILE = "wiki_corpus_150mb.txt"
TARGET_SIZE_MB = 150
TARGET_SIZE_BYTES = TARGET_SIZE_MB * 1024 * 1024  # 150 * 1024 * 1024

chunk = open(SRC_FILE, "rb").read()
chunk_size = len(chunk)

with open(DST_FILE, "wb") as out:
    written = 0
    while written < TARGET_SIZE_BYTES:
        out.write(chunk)
        written += chunk_size

print(f"[DONE] {DST_FILE} 생성 완료: {written / (1024 * 1024):.2f} MB")

