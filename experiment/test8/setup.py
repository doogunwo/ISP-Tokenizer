from setuptools import setup, find_packages

setup(
    name="nvme_file_lib",
    version="1.0.0",
    description="Python wrapper for NVMe file operations using C library",
    packages=find_packages(),
    include_package_data=True,
    package_data={
        "": ["nvme_file.so"],  # 공유 라이브러리 포함
    },
    zip_safe=False,
)

