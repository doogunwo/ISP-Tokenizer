from collections import defaultdict
from transformers import AutoTokenizer
import time
from tqdm import tqdm

class BPE():
    def __init__(self, corpus, vocab_size):
        self.corpus = corpus
        self.vocab_size = vocab_size
        self.time_data = []
        self.tokenizer = AutoTokenizer.from_pretrained("bert-base-uncased")
        self.word_freqs = defaultdict(int)
        self.splits = {}
        self.merges = {}

    def train(self):
        for text in self.corpus:
            words_with_offsets = self.tokenizer.backend_tokenizer.pre_tokenizer.pre_tokenize_str(text)
            new_words = [word for word, offset in words_with_offsets]
            for word in new_words:
                self.word_freqs[word] += 1

        alphabet = []
        for word in self.word_freqs.keys():
            for letter in word:
                if letter not in alphabet:
                    alphabet.append(letter)
        alphabet.sort()

        vocab = ["</w>"] + alphabet.copy()
        self.splits = {word: [c for c in word] for word in self.word_freqs.keys()}

        with tqdm(total=self.vocab_size - len(vocab), desc="Training BPE") as pbar:
            while len(vocab) < self.vocab_size:
                pair_freqs = self.compute_pair_freqs()
                if not pair_freqs:
                    print("No pairs to merge. Stopping training.")
                    break

                best_pair = max(pair_freqs, key=pair_freqs.get, default=None)
                if best_pair is None:
                    print("No valid best pair. Stopping training.")
                    break

                self.splits = self.merge_pair(*best_pair)
                self.merges[best_pair] = best_pair[0] + best_pair[1]  # 딕셔너리에 병합 규칙 저장
                vocab.append(best_pair[0] + best_pair[1])
                pbar.update(1)

    def compute_pair_freqs(self):
        pair_freqs = defaultdict(int)
        for word, freq in self.word_freqs.items():
            split = self.splits[word]
            if len(split) == 1:
                continue
            for i in range(len(split) - 1):
                pair = (split[i], split[i + 1])
                pair_freqs[pair] += freq
        return pair_freqs

    def merge_pair(self, a, b):
        for word in self.word_freqs:
            split = self.splits[word]
            if len(split) == 1:
                continue
            i = 0
            while i < len(split) - 1:
                if split[i] == a and split[i + 1] == b:
                    split = split[:i] + [a + b] + split[i + 2 :]
                else:
                    i += 1
            self.splits[word] = split
        return self.splits

    def save_merge_rules(self, file_path):
        """Save the merge rules (stored in self.merges) to a file."""
        try:
            with open(file_path, "w", encoding="utf8") as f:
                for pair, merge in self.merges.items():
                    f.write(f"{pair[0]} {pair[1]} {merge}\n")
            print(f"Merge rules successfully saved to '{file_path}'")
        except Exception as e:
            print(f"Error saving merge rules: {e}")

