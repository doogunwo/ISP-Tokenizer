# save_tokenized_dataset.py

from datasets import load_from_disk
from transformers import GPT2Tokenizer

# 1. 원본 텍스트 데이터 로드
dataset = load_from_disk("/home/doogunwo/Desktop/dataset")  # text 컬럼 존재

# 2. 토크나이저 준비
tokenizer = GPT2Tokenizer.from_pretrained("gpt2")
tokenizer.pad_token = tokenizer.eos_token

# 3. 토크나이징 함수 정의
def tokenize_fn(example):
    return tokenizer(
        example["text"],
        truncation=True,
        max_length=512,
        padding="max_length"
    )

# 4. 토크나이징 실행 (멀티 프로세싱 병렬 처리 추천)
tokenized_dataset = dataset.map(
    tokenize_fn,
    batched=True,
    remove_columns=["text"],
    num_proc=4
)

# 5. Arrow 형식으로 디스크에 저장
tokenized_dataset.save_to_disk("/home/doogunwo/Desktop/tokenized_dataset")
print("✅ Pre-tokenized 데이터셋 저장 완료")
