from tokenizers import Tokenizer
from datasets import load_dataset
import numpy as np
import time

# Load tokenizer
tokenizer = Tokenizer.from_file("byte_level_bpe_model.json")

# Load evaluation dataset
dataset = load_dataset("wikitext", "wikitext-2-raw-v1", split="test")
texts = [t for t in dataset["text"] if len(t.strip()) > 0]

# 1. Vocabulary size
vocab_size = tokenizer.get_vocab_size()
print(f"📚 Vocabulary Size: {vocab_size}")

# 2. Average characters per token
total_chars, total_tokens = 0, 0
for text in texts:
    total_chars += len(text)
    total_tokens += len(tokenizer.encode(text).tokens)
avg_chars_per_token = total_chars / total_tokens
print(f"🔎 Average Characters per Token: {avg_chars_per_token:.4f}")

# 3. Sequence length distribution
seq_lengths = [len(tokenizer.encode(text).tokens) for text in texts]
print("📏 Sequence Length Distribution")
print(f" - Mean   : {np.mean(seq_lengths):.2f}")
print(f" - Median : {np.median(seq_lengths):.2f}")
print(f" - Max    : {np.max(seq_lengths)}")
print(f" - Min    : {np.min(seq_lengths)}")
print(f" - StdDev : {np.std(seq_lengths):.2f}")

# 4. Processing speed (tokens per second)
start_time = time.time()
for text in texts:
    tokenizer.encode(text)
elapsed_time = time.time() - start_time
tokens_per_second = total_tokens / elapsed_time
print(f"⚡ Processing Speed: {tokens_per_second:,.2f} tokens/sec")

