from bpe import BPE
import time
import sys

# CLI 입력을 통해 파일 경로 받기
if len(sys.argv) != 2:
    print("Usage: python script.py <path_to_text_file>")
    sys.exit(1)
    
file_path = sys.argv[1]

# 1) 텍스트 파일 로드
try:
    with open(file_path, encoding="utf8") as f:
        corpus = f.readlines()
except FileNotFoundError:
    print(f"Error: File '{file_path}' not found.")
    sys.exit(1)

# 2) vocab_size 설정
vocab_size = 30000

# 3) BPE 객체 생성
MyBPE = BPE(corpus=corpus, vocab_size=vocab_size)

# 4) 학습
MyBPE.train()

# 5) 각 단계별 시간 출력

# 6) 테스트 문장 토큰화
text = ("Love, hate, or feel meh about Harry Potter, it’s hard to argue that J.K. Rowling..."
        " (이하 생략) ...")
print(f"\nBPE tokenization result of text:\n'{text}'")
pair_freqs = MyBPE.compute_pair_freqs()  # Compute the frequencies of character pairs
print(f"Number of key-pairs: {len(pair_freqs)}")