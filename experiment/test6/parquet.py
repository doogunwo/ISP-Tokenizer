import pandas as pd

# 파일 경로
excel_file = 'dataset1.csv'  # 원본 Excel 파일 경로
output_kor_parquet = 'kor.parquet'  # 한국어 Parquet 파일 경로
output_eng_parquet = 'eng.parquet'  # 영어 Parquet 파일 경로

# Excel 파일 로드
try:
    data = pd.read_excel(excel_file, engine='openpyxl')  # 엔진 명시적으로 지정
except UnicodeDecodeError as e:
    print(f"UnicodeDecodeError: {e}")
    print("Ensure the file encoding is compatible.")
    exit(1)

# 200,000행까지만 사용 (맨 아래 836개 무시)
data = data.iloc[:200000]

# 한국어와 영어 열 추출
kor_data = data['원문']
eng_data = data['번역문']

# 각각 Parquet 파일로 저장
kor_data.to_frame(name='kor').to_parquet(output_kor_parquet, index=False)
eng_data.to_frame(name='eng').to_parquet(output_eng_parquet, index=False)

print(f"Parquet files created: {output_kor_parquet}, {output_eng_parquet}")
