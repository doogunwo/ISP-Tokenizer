import time
import re
from tokenizers import Tokenizer

local_data = "train.txt"
with open(local_data, "r", encoding="utf-8") as f:
        lines = f.readlines()

