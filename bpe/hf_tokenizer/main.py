from tokenizers import Tokenizer
from tokenizers.models import BPE
from tokenizers.trainers import BpeTrainer
from tokenizers.pre_tokenizers import ByteLevel  # ✅ 바이트 레벨 토크나이저 사용

# ✅ Byte-Level BPE 토크나이저 초기화
tokenizer = Tokenizer(BPE(unk_token="[UNK]"))
tokenizer.pre_tokenizer = ByteLevel()  # ✅ 바이트 단위 토큰화 적용

# ✅ 학습 설정 (바이트 기반)
trainer = BpeTrainer(
    vocab_size=30000,  # 서브워드 개수
    min_frequency=2,    # 최소 빈도
    special_tokens=["[PAD]", "[UNK]", "[CLS]", "[SEP]", "[MASK]"]
)

# ✅ 위키 코퍼스에서 학습 진행
corpus_path = "wiki_corpus.txt"
tokenizer.train([corpus_path], trainer)

# ✅ 토크나이저 저장
tokenizer.save("bbpe_tokenizer.bin")
print("Byte-Level BPE 토크나이저 학습 완료! 🚀🔥")

