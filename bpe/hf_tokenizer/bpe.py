from tokenizers import Tokenizer, models, trainers, pre_tokenizers, processors, decoders

# 1. 원본 텍스트 데이터 (가상의 텍스트)
original_texts = [
    "This is LBA 0 data. It is stored in raw binary format.",
    "LBA 1 contains another piece of data. BPE will learn from this.",
    "More data in LBA 2. Tokenization will segment this efficiently.",
    "LBA 3 final data block. BPE encoding applies here as well."
]

# 2. 텍스트를 바이너리로 변환 (NVMe LBA 저장 방식과 유사)
binary_data = [text.encode('utf-8') for text in original_texts]

# 3. BPE 모델 생성 (Byte-Level BPE 적용)
tokenizer = Tokenizer(models.BPE())

# 4. Byte-Level Pre-tokenizer 설정
tokenizer.pre_tokenizer = pre_tokenizers.ByteLevel(add_prefix_space=False)

# 5. BPE 학습을 위한 Trainer 설정 (Byte-Level 적용)
trainer = trainers.BpeTrainer(
    special_tokens=["[UNK]", "[CLS]", "[SEP]", "[PAD]", "[MASK]"], 
    show_progress=True, 
    initial_alphabet=pre_tokenizers.ByteLevel.alphabet()  # Byte 단위 학습
)

# 6. train_from_iterator()에 binary_data를 문자열 리스트로 변환 후 전달
binary_str_data = [data.decode('utf-8') for data in binary_data]  # 🔥 여기서 바이너리를 문자열로 변환
tokenizer.train_from_iterator(binary_str_data, trainer)

# 7. Byte-Level Decoding 설정
tokenizer.post_processor = processors.ByteLevel(trim_offsets=False)
tokenizer.decoder = decoders.ByteLevel()

# 8. 테스트 문장 (바이너리를 문자열로 변환 후 인코딩)
test_sentence = b"LBA tokenization example.".decode('utf-8')  # 🔥 오류 해결: bytes → str 변환
encoded = tokenizer.encode(test_sentence)

# 9. 결과 출력
print("Encoded Tokens:", encoded.tokens)
print("Decoded Text:", tokenizer.decode(encoded.ids))

