import json

# ✅ bbpe_tokenizer.json 로드
with open("bbpe_tokenizer.json", "r", encoding="utf-8") as f:
    tokenizer_data = json.load(f)

# ✅ Vocab & Merges 추출
vocab = tokenizer_data["model"]["vocab"]
merges = tokenizer_data["model"]["merges"]

# ✅ JSON 파일로 저장
with open("vocab.json", "w", encoding="utf-8") as vocab_file:
    json.dump(vocab, vocab_file, indent=2, ensure_ascii=False)

with open("merges.json", "w", encoding="utf-8") as merges_file:
    json.dump(merges, merges_file, indent=2, ensure_ascii=False)

print("✅ vocab.json 및 merges.json 저장 완료! 🚀🔥")

