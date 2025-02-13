from tokenizers import Tokenizer
from tokenizers.models import BPE
from tokenizers.trainers import BpeTrainer
from tokenizers.pre_tokenizers import ByteLevel  # ✅ 바이트 레벨 토크나이저 사용
import json
tokenizer = Tokenizer(BPE(unk_token="[UNK]"))
tokenizer.pre_tokenizer = ByteLevel()  # ✅ 바이트 단위 토큰화 적용

trainer = BpeTrainer(
    vocab_size=30000,  # 서브워드 개수
    min_frequency=2,    # 최소 빈도
    special_tokens=["[PAD]", "[UNK]", "[CLS]", "[SEP]", "[MASK]"]
)

corpus_path = "wiki_corpus.txt"
tokenizer.train([corpus_path], trainer)

vocab = tokenizer.get_vocab()
merge = tokenizer.model.merges

with open("vocab.json", "w", encoding="utf-8") as vocab_file:
    json.dump(vocab, vocab_file, indent=2, ensure_ascii=False)

with open("merges.json", "w", encoding="utf-8") as merges_file:
    json.dump(merge, merges_file, indent=2, ensure_ascii=False)

tokenizer.save("bbpe_tokenizer.bin")
print("Byte-Level BPE 토크나이저 학습 완료! 🚀🔥")

