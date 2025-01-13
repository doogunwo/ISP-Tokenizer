
from nvme_wrapper import NVMeFile
if __name__ == "__main__":
    try:
        nvme_file = NVMeFile("example.txt", "r")
        content = nvme_file.read(256)
        print("읽은 내용:", content)
        nvme_file.close()
    except FileNotFoundError as e:
        print(e)

