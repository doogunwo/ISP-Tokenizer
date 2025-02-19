from tokenizers import Tokenizer
from tokenizers.models import BPE
from tokenizers.trainers import BpeTrainer
from tokenizers.pre_tokenizers import ByteLevel  # ✅ 바이트 레벨 토크나이저 사용
import json
tokenizer = Tokenizer(BPE(unk_token="[UNK]"))
tokenizer.pre_tokenizer = ByteLevel()  # ✅ 바이트 단위 토큰화 적용

trainer = BpeTrainer(
    vocab_size=30000,  # 서브워드 개수
    min_frequency=4,    # 최소 빈도
    special_tokens=["[PAD]", "[UNK]", "[CLS]", "[SEP]", "[MASK]"]
)

corpus_path = "wiki_corpus.txt"
tokenizer.train([corpus_path], trainer)

tokenizer.save("byte_level_bpe_model.json")
print("Byte-Level BPE 토크나이저 학습 완료! 🚀🔥")

