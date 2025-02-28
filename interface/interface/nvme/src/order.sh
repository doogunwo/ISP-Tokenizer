#!/bin/bash
sudo nvme io-passthru /dev/nvme0n1 \
    --opcode=0xD4 \
    --namespace-id=1 \
    --data-len=8192 \
    --cdw10=31932 \
    --cdw11=261 \
    --read 
