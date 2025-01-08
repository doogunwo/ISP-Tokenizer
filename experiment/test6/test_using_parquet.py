import pandas as pd
import time
import sys
from bpe import BPE

# CLI 입력을 통해 Parquet 파일 경로와 사용할 열 이름 받기
if len(sys.argv) != 3:
    print("Usage: python script.py <path_to_parquet_file> <column_name>")
    sys.exit(1)

file_path = sys.argv[1]
column_name = sys.argv[2]

# 1) Parquet 파일 로드
try:
    data = pd.read_parquet(file_path)
    corpus = data[column_name].tolist()[:3000]  # 0~3000개만 사용
    print(len(corpus))
    
except FileNotFoundError:
    print(f"Error: File '{file_path}' not found.")
    sys.exit(1)
except KeyError:
    print(f"Error: Column '{column_name}' not found in the Parquet file. Available columns: {list(data.columns)}")
    sys.exit(1)

# 2) vocab_size 설정
vocab_size = 50000

# 3) BPE 객체 생성
MyBPE = BPE(corpus=corpus, vocab_size=vocab_size)

# 4) 학습
merges = MyBPE.train()
if len(merges) == 0:
    print("No merges were performed. Please check your corpus and vocab_size settings.")

# 5) 각 단계별 시간 출력
time_data = MyBPE.time_data
print("\n=== Training time breakdown ===")
print(f"1) Reading corpus:        {time_data[0]:.4f} sec")
print(f"2) Building alphabet:     {time_data[1]:.4f} sec")
print(f"3) Init vocab & splits:   {time_data[2]:.4f} sec")
print(f"4) Pair frequency total:  {time_data[3]:.4f} sec")
print(f"5) Best pair selection:   {time_data[4]:.4f} sec")
print(f"6) Merging pairs:         {time_data[5]:.4f} sec")
print(f"7) Total training time:   {time_data[6]:.4f} sec")

# 6) 테스트 문장 토큰화
text = ("Love, hate, or feel meh about Harry Potter, it’s hard to argue that J.K. Rowling..."
        " (이하 생략) ...")
print(f"\nBPE tokenization result of text:\n'{text}'")
print(MyBPE.tokenize(text))