import sys
import pandas as pd

# Function to analyze text data
def analyze_text_file(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        data = f.readlines()
    total_lines = len(data)
    total_words = sum(len(line.split()) for line in data)
    total_chars = sum(len(line) for line in data)

    print("=== Text File Analysis ===")
    print(f"Total lines: {total_lines}")
    print(f"Total words: {total_words}")
    print(f"Total characters: {total_chars}")

# Function to analyze Parquet data
def analyze_parquet_file(file_path):
    df = pd.read_parquet(file_path)
    total_rows = len(df)
    total_columns = len(df.columns)
    word_counts = df.apply(lambda row: row.astype(str).str.split().map(len).sum(), axis=1).sum()
    char_counts = df.apply(lambda row: row.astype(str).str.len().sum(), axis=1).sum()

    print("=== Parquet File Analysis ===")
    print(f"Total rows: {total_rows}")
    print(f"Total columns: {total_columns}")
    print(f"Total words: {word_counts}")
    print(f"Total characters: {char_counts}")

# Main function to handle file analysis
def main():
    if len(sys.argv) != 2:
        print("Usage: python3 analyze.py <path>")
        sys.exit(1)

    file_path = sys.argv[1]

    try:
        if file_path.endswith('.txt'):
            analyze_text_file(file_path)
        elif file_path.endswith('.parquet'):
            analyze_parquet_file(file_path)
        else:
            print("Unsupported file format. Please provide a .txt or .parquet file.")
    except FileNotFoundError:
        print(f"Error: File '{file_path}' not found.")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()