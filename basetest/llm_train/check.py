import torch

def check_gpu():
    print("==== GPU 상태 점검 ====\n")

    # GPU 사용 가능 여부
    if not torch.cuda.is_available():
        print("❌ GPU 사용 불가: torch.cuda.is_available() == False")
        return

    # 사용 가능한 GPU 수
    gpu_count = torch.cuda.device_count()
    print(f"✅ 사용 가능한 GPU 개수: {gpu_count}")

    for i in range(gpu_count):
        print(f"\n--- GPU {i} ---")
        print(f"이름          : {torch.cuda.get_device_name(i)}")
        print(f"총 메모리     : {torch.cuda.get_device_properties(i).total_memory / (1024**2):.1f} MB")
        print(f"현재 사용 메모리 : {torch.cuda.memory_allocated(i) / (1024**2):.1f} MB")
        print(f"캐시된 메모리   : {torch.cuda.memory_reserved(i) / (1024**2):.1f} MB")
        print(f"현재 디바이스?  : {'✅' if i == torch.cuda.current_device() else '❌'}")

    print("\n=========================")

if __name__ == "__main__":
    check_gpu()
