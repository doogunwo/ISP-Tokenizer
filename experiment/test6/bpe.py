from collections import Counter, defaultdict
from transformers import AutoTokenizer
import time
from tqdm import tqdm

class BPE():
    """Byte-Pair Encoding: Subword-based tokenization algorithm."""
    
    def __init__(self, corpus, vocab_size):
        """Initialize BPE tokenizer."""
        self.corpus = corpus
        self.vocab_size = vocab_size
        self.time_data = []
        # pre-tokenize the corpus into words, BERT pre-tokenizer is used here
        self.tokenizer = AutoTokenizer.from_pretrained("bert-base-uncased")
        self.word_freqs = defaultdict(int)
        self.splits = {}
        self.merges = {}

    def train(self):
        """Train BPE tokenizer."""
        start_all = time.time()
        start_corpus = time.time()
        # 말뭉치에 있는 각 단어의 빈도를 계산합니다.
        for text in self.corpus:
            words_with_offsets = self.tokenizer.backend_tokenizer.pre_tokenizer.pre_tokenize_str(text)
            new_words = [word for word, offset in words_with_offsets]
            for word in new_words:
                self.word_freqs[word] += 1
        end_corpus = time.time() 
        self.time_data.append(end_corpus-start_corpus)
        
        start_letter = time.time()
        # 말뭉치에 있는 모든 문자의 기본 어휘를 계산합니다.
        alphabet = []
        for word in self.word_freqs.keys():
            for letter in word:
                if letter not in alphabet:
                    alphabet.append(letter)
        alphabet.sort()
        end_letter = time.time()
        self.time_data.append(end_letter-start_letter)
        
        start_vocab = time.time()
        # 어휘 시작 부분에 특수 토큰을 추가하십시오 </w>
        vocab = ["</w>"] + alphabet.copy()

        #훈련 전에 각 단어를 개별 문자로 분할
        self.splits = {word: [c for c in word] for word in self.word_freqs.keys()}
        end_vocab = time.time()
        self.time_data.append(end_vocab-start_vocab)
        
        time_pair_freqs = 0
        time_best_pair = 0
        time_merge_pair=0
        
        with tqdm(total=self.vocab_size - len(vocab), desc="Training BPE") as pbar:
            while len(vocab) < self.vocab_size:
                # Compute pair frequencies
                start = time.time()
                pair_freqs = self.compute_pair_freqs()
                end = time.time()
                time_pair_freqs += (end - start)

                if not pair_freqs:
                    print("No pairs to merge. Stopping training.")
                    break

                # Find the best pair
                start = time.time()
                best_pair = max(pair_freqs, key=pair_freqs.get, default=None)
                if best_pair is None:
                    print("No valid best pair. Stopping training.")
                    break
                end = time.time()
                time_best_pair += (end - start)

                # Merge the best pair
                start = time.time()
                self.splits = self.merge_pair(*best_pair)
                self.merges[best_pair] = best_pair[0] + best_pair[1]
                vocab.append(best_pair[0] + best_pair[1])
                end = time.time()
                time_merge_pair += (end - start)

                pbar.update(1)
            
        end_all = time.time()
        self.time_data.append(time_pair_freqs)
        self.time_data.append(time_best_pair)
        self.time_data.append(time_merge_pair)
        self.time_data.append(end_all-start_all)
        
        return self.merges


    def compute_pair_freqs(self):
        """Compute the frequency of each pair."""

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
        """Merge the given pair."""

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
    

    def tokenize(self, text):
        """Tokenize a given text with trained BPE tokenizer (including pre-tokenization, split, and merge)."""
        
        pre_tokenize_result = self.tokenizer._tokenizer.pre_tokenizer.pre_tokenize_str(text)
        pre_tokenized_text = [word for word, offset in pre_tokenize_result]
        splits_text = [[l for l in word] for word in pre_tokenized_text]

        for pair, merge in self.merges.items():
            for idx, split in enumerate(splits_text):
                i = 0
                while i < len(split) - 1:
                    if split[i] == pair[0] and split[i + 1] == pair[1]:
                        split = split[:i] + [merge] + split[i + 2 :]
                    else:
                        i += 1
                splits_text[idx] = split
        result = sum(splits_text, [])
        return result