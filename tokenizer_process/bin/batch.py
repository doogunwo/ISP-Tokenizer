import subprocess
import re
import csv
import sys
import time
EXEC_PATH = "./bpe_host_copy"
FILE_PATH = "/mnt/nvme/wiki.128kb.txt"
# 로그에서 퍼포먼스 지표와 시간 추출
def parse_output(output):
    perf_data = {
        "bpe_token_time_ms": None,
        "model_load_time_ms": None,
        "instructions": None,
        "cpu_cycles": None,
        "branch_instr": None,
        "branch_miss": None,
        "l1d_access": None,
        "l1d_miss": None,
        "l3_access": None,
        "l3_miss": None
    }

    for line in output.splitlines():
        if "토큰화 처리 시간 " in line:
            match = re.search(r"([\d.]+)", line)
            if match:
                perf_data["bpe_token_time_ms"] = float(match.group(1))
        elif "모델 로딩 시간" in line:
            match = re.search(r"([\d.]+)", line)
            if match:
                perf_data["model_load_time_ms"] = float(match.group(1))
        elif "Instructions" in line:
            perf_data["instructions"] = int(line.split(":")[1].strip())
        elif "CPU Cycles" in line:
            perf_data["cpu_cycles"] = int(line.split(":")[1].strip())
        elif "Branch Instructions" in line:
            perf_data["branch_instr"] = int(line.split(":")[1].strip())
        elif "Branch Misses" in line:
            perf_data["branch_miss"] = int(line.split(":")[1].strip())
        elif "L1D Access" in line:
            perf_data["l1d_access"] = int(line.split(":")[1].strip())
        elif "L1D Miss" in line:
            perf_data["l1d_miss"] = int(line.split(":")[1].strip())
        elif "L3 Access" in line:
            perf_data["l3_access"] = int(line.split(":")[1].strip())
        elif "L3 Miss" in line:
            perf_data["l3_miss"] = int(line.split(":")[1].strip())

    return perf_data

def run_once():
    result = subprocess.run(["sudo", EXEC_PATH, FILE_PATH], capture_output=True, text=True)
    return parse_output(result.stdout)

def benchmark(repeats, output_csv):
    with open(output_csv, 'w', newline='') as csvfile:
        fieldnames = [
            "run", "bpe_token_time_ms", "model_load_time_ms",
            "instructions", "cpu_cycles", "branch_instr", "branch_miss",
            "l1d_access", "l1d_miss", "l3_access", "l3_miss"
        ]
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()

        for i in range(1, repeats + 1):
            print(f"[RUN] {i}/{repeats}")
            data = run_once()
            data["run"] = i
            writer.writerow(data)
            time.sleep(0.1)

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("사용법: python3 benchmark_perf.py <반복횟수> <결과.csv>")
        sys.exit(1)

    count = int(sys.argv[1])
    output_file = sys.argv[2]
    benchmark(count, output_file)

