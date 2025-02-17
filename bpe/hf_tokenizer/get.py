from transformers import AutoTokenizer

# 원하는 모델 선택 (BPE 기반 모델)
MODEL_NAME = "gpt2"  # 또는 "roberta-base", "openai/clip-vit-base-patch32"

# Tokenizer 다운로드
tokenizer = AutoTokenizer.from_pretrained(MODEL_NAME)

# vocab.json과 merges.txt 저장
tokenizer.save_pretrained("./tokenizer_files")

print("✅ vocab.json & merges.txt saved in ./tokenizer_files/")

