import subprocess
import time

CMD = ["sudo", "./main_ndp", "/mnt/nvme/wiki.txt"]
REPEAT = 400
SLEEP_SEC = 1

for i in range(1, REPEAT + 1):
    print(f"[RUN] {i} / {REPEAT}")
    result = subprocess.run(CMD, capture_output=True, text=True)
    
    print(result.stdout)
    if result.stderr:
        print("[ERROR]", result.stderr)

    time.sleep(SLEEP_SEC)

