#!/usr/bin/env python3
"""按仓库锁定版本安装 SDK，只使用本地目录或本地压缩包，不下载依赖。"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[1]
EXAMPLES = ("p4_iot_basic", "c3_iot_basic", "c5_iot_basic")

def example_lock(name):
    path = ROOT / "examples" / name / "sdk.lock.json"
    if name == "p4_iot_basic":
        path = ROOT / "sdk.lock.json"
    return json.loads(path.read_text())


def verify(sdk, lock):
    """先验证清单本身，再验证清单覆盖的全部文件。"""
    manifest_path = sdk / "sdk-manifest.json"
    if hashlib.sha256(manifest_path.read_bytes()).hexdigest() != lock["manifest_sha256"]:
        raise ValueError("SDK 清单与 sdk.lock.json 不一致，请使用匹配的 SDK 包")
    manifest = json.loads(manifest_path.read_text())
    if manifest["target"] != lock["target"]:
        raise ValueError("SDK 芯片与例程不匹配")
    for name, digest in manifest["files_sha256"].items():
        path = sdk / name
        if not path.resolve().is_relative_to(sdk.resolve()) or not path.is_file():
            raise ValueError("SDK 文件路径无效：" + name)
        if hashlib.sha256(path.read_bytes()).hexdigest() != digest:
            raise ValueError("SDK 文件校验失败：" + name)


def install(source, destination, lock):
    verify(source, lock)
    if destination.exists():
        # 相同版本重复安装不覆盖文件；不一致时交给用户在新工作区处理。
        verify(destination, lock)
        print("已安装匹配版本，校验通过：", destination)
        return
    # 先完整复制并验证暂存目录，成功后才让工程看到安装结果。
    with tempfile.TemporaryDirectory(prefix=".sdk-install-", dir=destination.parent) as temporary:
        staged = Path(temporary) / "gagent-sdk"
        shutil.copytree(source, staged)
        verify(staged, lock)
        staged.rename(destination)
    print("SDK 安装并校验完成：", destination)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--sdk-dir", type=Path, help="已解压的 SDK 根目录")
    source.add_argument("--archive", type=Path, help="本地 SDK tar.gz 包，需要 Python 3.12 或更新版本")
    source.add_argument("--verify-only", action="store_true", help="仅校验当前已安装的 SDK")
    parser.add_argument("--example", choices=EXAMPLES, help="指定例程；仅校验时默认全部，安装时默认 p4_iot_basic")
    args = parser.parse_args()
    name = args.example or "p4_iot_basic"
    destination = ROOT / "examples" / name / "gagent-sdk"
    try:
        if args.verify_only:
            for example in ((args.example,) if args.example else EXAMPLES):
                verify(ROOT / "examples" / example / "gagent-sdk", example_lock(example))
                print(example + "：本地 SDK 完整性校验通过")
        elif args.sdk_dir:
            install(args.sdk_dir.resolve(), destination, example_lock(name))
        else:
            # data 过滤器拒绝目录穿越及危险的归档条目，禁止直接解压到工程。
            with tempfile.TemporaryDirectory(prefix="gagent-sdk-unpack-") as temporary:
                unpacked = Path(temporary)
                with tarfile.open(args.archive, "r:gz") as archive:
                    archive.extractall(unpacked, filter="data")
                sdk = unpacked / "gagent-sdk"
                if not sdk.is_dir():
                    raise ValueError("压缩包中缺少 gagent-sdk 根目录")
                install(sdk, destination, example_lock(name))
    except (OSError, ValueError, KeyError, tarfile.TarError) as error:
        parser.exit(1, "SDK 安装失败：" + str(error) + "\n")


if __name__ == "__main__":
    main()
