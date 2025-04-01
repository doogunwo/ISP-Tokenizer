#!/bin/bash


# 1️⃣ 기존 마운트 해제
sudo umount /mnt/nvme 2>/dev/null

# 2️⃣ 기존 연결 해제
sudo nvme disconnect-all

# 3️⃣ NVMe-over-TCP 디바이스 검색
sudo nvme discover -t tcp -a 10.125.68.230 -s 4420

# 4️⃣ NVMe 연결
sudo nvme connect -t tcp -n nqn.2025-01.io.spdk:cnode1 -a 10.125.68.230 -s 4420

# 5️⃣ 파일 시스템 확인
FSTYPE=$(lsblk -no FSTYPE /dev/nvme0n1)

if [ -z "$FSTYPE" ]; then
    echo "[ERROR] No filesystem found. Formatting as ext4..."
    sudo mkfs.ext4 /dev/nvme0n1
fi

# 6️⃣ 마운트
sudo mount /dev/nvme0n1 /mnt/nvme || { echo "[ERROR] Mount failed"; exit 1; }

# 7️⃣ 파일 복사
sudo cp wiki_corpus.txt /mnt/nvme
echo "[SUCCESS] File copied to NVMe!"

