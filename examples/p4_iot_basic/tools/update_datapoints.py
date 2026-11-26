#!/usr/bin/env python3
"""更新数据点协议，真实 PK 仅从本地配置读取，不写入生成源码或快照。"""
import argparse
import importlib.util
import json
from pathlib import Path
import re
import tempfile

PROJECT = Path(__file__).resolve().parents[1]
PUBLIC_KEY = "0" * 32


def update(project, datapoints):
    main = project / "main"
    document = json.loads(datapoints.read_text())
    if "entities" not in document and isinstance(document.get("datas"), dict):
        document = document["datas"]
    product_key = document.get("product_key", "")
    if product_key != PUBLIC_KEY:
        secrets = main / "product_secrets.hpp"
        match = re.search(r'^\s*#define\s+GAGENT_SOC_PRODUCT_KEY\s+"([0-9a-fA-F]{32})"',
                          secrets.read_text() if secrets.exists() else "", re.MULTILINE)
        if not match or product_key.lower() != match[1].lower():
            raise ValueError("数据点 PK 与 main/product_secrets.hpp 不一致，请确认产品配置")
    # 同时移除 JSON 其他字符串中对该 PK 的引用。
    document = json.loads(json.dumps(document, ensure_ascii=False).replace(product_key, PUBLIC_KEY))
    spec = importlib.util.spec_from_file_location(
        "sdk_soc_generator", project / "gagent-sdk/tools/generate_soc_board.py")
    generator = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(generator)
    manifest = json.loads((main / "generation.json").read_text())
    manifest["product_key"] = PUBLIC_KEY
    # SDK 原包不修改；先在临时目录生成，再适配为本地凭据引用。
    with tempfile.TemporaryDirectory(prefix="gagent-datapoints-") as temporary:
        staging = Path(temporary)
        (staging / "main").mkdir()
        (staging / "main/generation.json").write_text(json.dumps(manifest))
        generator.update_project(PUBLIC_KEY, staging, document)
        generated = {p.name: p.read_text() for p in (staging / "main").iterdir()}
    declaration = f'inline constexpr char product_key[] = "{PUBLIC_KEY}";'
    header = generated["gizwits_protocol.hpp"]
    if header.count(declaration) != 1:
        raise ValueError("SDK 生成器格式变化，未写入文件；请适配本地凭据引用")
    generated["gizwits_protocol.hpp"] = header.replace(
        declaration, 'inline constexpr char product_key[] = GAGENT_SOC_PRODUCT_KEY;'
    ).replace('#include "gagent/protocol/protocol.hpp"',
              '#include "product_config.hpp"\n#include "gagent/protocol/protocol.hpp"')
    if any((main / name).is_symlink() for name in generated):
        raise ValueError("拒绝覆盖符号链接")
    for name, content in generated.items():
        (main / name).write_text(content)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--datapoints", type=Path, default=PROJECT / "main/datapoints.json")
    args = parser.parse_args()
    try:
        update(PROJECT, args.datapoints)
    except (OSError, ValueError, KeyError, TypeError) as error:
        # 不输出 JSON 内容或底层异常，避免把真实 PK 带进终端日志。
        parser.exit(1, "更新失败：请检查本地 PK、数据点格式、SDK 与生成记录。\n")
    print("数据点已更新；生成文件不含真实 PK，凭据仍由 product_secrets.hpp 提供。")


if __name__ == "__main__":
    main()
