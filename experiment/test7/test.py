import sys
from bpe import BPE

def main():
    # CLI 인자를 확인
    if len(sys.argv) != 3:
        print("Usage: python test.py <path_to_text_file> <path_to_save_merge_rules>")
        sys.exit(1)

    text_file = sys.argv[1]
    merge_rules_file = sys.argv[2]

    # 텍스트 파일 읽기
    try:
        with open(text_file, "r", encoding="utf8") as f:
            corpus = f.readlines()
        print(f"Loaded corpus from '{text_file}', total lines: {len(corpus)}")
    except FileNotFoundError:
        print(f"Error: File '{text_file}' not found.")
        sys.exit(1)

    # BPE 학습
    vocab_size = 25000  # 원하는 vocab_size 설정
    bpe_tokenizer = BPE(corpus=corpus, vocab_size=vocab_size)
    print("Training BPE tokenizer...")
    bpe_tokenizer.train()

    # 병합 규칙 저장
    print(f"Saving merge rules to '{merge_rules_file}'...")
    bpe_tokenizer.save_merge_rules(merge_rules_file)

    # 테스트 문장
    test_sentence = (
        "Love, hate, or feel meh about Harry Potter, it's hard to argue that J.K. Rowling "
        "has created a captivating world."
    )
    print(f"\nTest Sentence: {test_sentence}")

    # BPE 토큰화
    tokenized = bpe_tokenizer.tokenize(test_sentence)
    print(f"\nTokenized Result:\n{tokenized}")

if __name__ == "__main__":
    main()

