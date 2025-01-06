from bpe import BPE
import time

# 1) 위키 코퍼스 로드
with open('wiki_corpus.txt', encoding="utf8") as f:
    corpus = f.readlines()
    print(corpus[:5])  # 일부 미리 확인

# 2) vocab_size 설정
vocab_size = 1000

# 3) BPE 객체 생성
MyBPE = BPE(corpus=corpus, vocab_size=vocab_size)

# 4) 학습
MyBPE.train()

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
