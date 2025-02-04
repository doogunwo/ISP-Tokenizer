from tokenizers import Tokenizer

# ✅ 저장된 토크나이저 로드
tokenizer = Tokenizer.from_file("bpe_tokenizer.json")

# ✅ 테스트 문장
test_sentence = "Hugging Face provides great NLP tools!"

# ✅ 인코딩
encoded = tokenizer.encode(test_sentence)
print("Encoded IDs:", encoded.ids)
print("Encoded Tokens:", encoded.tokens)

# ✅ 디코딩
decoded = tokenizer.decode(encoded.ids)
print("Decoded:", decoded)
