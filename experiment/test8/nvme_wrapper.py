import ctypes
import os

# 공유 라이브러리 경로
LIB_PATH = os.path.join(os.path.dirname(__file__), "nvme_file.so")

# 동적 라이브러리 로드
nvme_lib = ctypes.CDLL(LIB_PATH)

# C 함수 프로토타입 선언
nvme_lib.nvme_open.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
nvme_lib.nvme_open.restype = ctypes.c_void_p

nvme_lib.nvme_read.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_size_t]
nvme_lib.nvme_read.restype = ctypes.c_size_t

nvme_lib.nvme_close.argtypes = [ctypes.c_void_p]
nvme_lib.nvme_close.restype = None

# NVMeFile 래퍼 클래스
class NVMeFile:
    def __init__(self, path, mode):
        self.file = nvme_lib.nvme_open(path.encode('utf-8'), mode.encode('utf-8'))
        if not self.file:
            raise FileNotFoundError(f"Failed to open file: {path}")

    def read(self, size):
        buffer = ctypes.create_string_buffer(size)
        read_size = nvme_lib.nvme_read(self.file, buffer, size)
        return buffer.raw[:read_size].decode('utf-8')

    def close(self):
        if self.file:
            nvme_lib.nvme_close(self.file)
            self.file = None

