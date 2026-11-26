#!/usr/bin/env python3
"""根据本地 PK/PS 生成独立例程的数据点协议；不覆盖用户业务代码。"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import tempfile
import urllib.error
import urllib.request

MAX_DOCUMENT_BYTES = 2 * 1024 * 1024
# 平台公开 PK 查询接口使用 HTTP；HTTPS 同路径目前跳转到开发者首页。
# 可用 --datapoints 完全离线生成，PS 从不发送到网络。
API = "http://site.gizwits.com/v2/datapoint?product_key="


class ProductError(ValueError):
    """可直接展示给开发者、不含凭据的错误。"""


def credentials(path):
    if not path.is_file():
        raise ProductError("请先复制 main/product_secrets.example.hpp 为 main/product_secrets.hpp，填写 PK/PS。")
    text = path.read_text(encoding="utf-8")
    values = []
    for name in ("KEY", "SECRET"):
        declarations = re.findall(r"^\s*#\s*define\s+GAGENT_SOC_PRODUCT_" + name + r"\b([^\n]*)", text, re.M)
        match = re.fullmatch(r'\s+"([0-9a-fA-F]{32})"\s*(?://[^\n]*)?', declarations[0]) if len(declarations) == 1 else None
        if not match or match[1] == "0" * 32:
            label = "PK" if name == "KEY" else "PS"
            raise ProductError(f"请在 main/product_secrets.hpp 中填写一次 {label}，使用 32 位十六进制字符串，不能为全零。")
        values.append(match[1])
    return tuple(values)


class SameOriginRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, request, fp, code, msg, headers, newurl):
        from urllib.parse import urlsplit
        parsed = urlsplit(newurl)
        if parsed.scheme not in ("http", "https") or parsed.netloc != "site.gizwits.com":
            raise ProductError("数据点接口重定向到其他站点，请改用 --datapoints 导入平台 JSON。")
        return super().redirect_request(request, fp, code, msg, headers, newurl)


def load_document(pk, path=None):
    if path is not None:
        with path.open("rb") as stream:
            raw = stream.read(MAX_DOCUMENT_BYTES + 1)
    else:
        # 公开数据点接口仅使用 PK；PS 不会进入 URL、请求头或请求体。
        request = urllib.request.Request(API + pk, headers={"Accept": "application/json"})
        try:
            with urllib.request.build_opener(SameOriginRedirect()).open(request, timeout=30) as response:
                raw = response.read(MAX_DOCUMENT_BYTES + 1)
        except urllib.error.HTTPError as error:
            raise ProductError(f"数据点接口返回 HTTP {error.code}，请检查 PK，或用 --datapoints 导入平台 JSON。") from None
        except (urllib.error.URLError, TimeoutError, OSError):
            raise ProductError("无法获取数据点，请检查网络/代理，或用 --datapoints 导入平台 JSON。") from None
    if len(raw) > MAX_DOCUMENT_BYTES:
        raise ProductError("数据点 JSON 超过 2 MiB 限制。")
    try:
        document = json.loads(raw)
    except (ValueError, UnicodeError):
        raise ProductError("数据点文件或接口响应不是有效 JSON。") from None
    if not isinstance(document, dict):
        raise ProductError("数据点 JSON 必须是对象。")
    if "entities" not in document and isinstance(document.get("datas"), dict):
        document = document["datas"]
    product_key = document.get("product_key")
    if not isinstance(product_key, str) or product_key.lower() != pk.lower():
        raise ProductError("数据点 JSON 的 PK 与本地配置不一致；原有生成文件未修改。")
    return document


def load_sample(project, pk):
    sample = project / "sample"
    manifest = json.loads((sample / "manifest.json").read_text(encoding="utf-8"))
    if manifest.get("product_key_sha256") != hashlib.sha256(pk.lower().encode()).hexdigest():
        raise ProductError("本地 PK 与随仓库样例的产品不一致；自己的产品请在线生成或用 --datapoints 导入。")
    for name in ("datapoints.json", "gizwits_protocol.hpp", "gizwits_protocol.cc"):
        if hashlib.sha256((sample / name).read_bytes()).hexdigest() != manifest["files_sha256"].get(name):
            raise ProductError("随仓库数据点样例不完整或已修改，请重新导出样例。")
    document = json.loads((sample / "datapoints.json").read_text(encoding="utf-8"))
    if document.get("product_key") != "0" * 32:
        raise ProductError("公开样例的 PK 必须是全零占位符。")
    document["product_key"] = pk
    return document


def write_directory(destination, outputs):
    if destination.is_symlink():
        raise ProductError("输出目录不能是符号链接。")
    with tempfile.TemporaryDirectory(prefix="." + destination.name + "-", dir=destination.parent) as temporary:
        staging = Path(temporary) / "next"
        backup = Path(temporary) / "previous"
        staging.mkdir(mode=0o700)
        for name, content in outputs.items():
            (staging / name).write_text(content, encoding="utf-8")
        if destination.exists():
            destination.rename(backup)
        try:
            staging.rename(destination)
        except OSError:
            if backup.exists():
                backup.rename(destination)
            raise


def export_sample(project):
    pk, ps = credentials(project / "main/product_secrets.hpp")
    generated = project / "main/generated"
    document = json.loads((generated / "datapoints.json").read_text(encoding="utf-8"))
    if document.get("product_key", "").lower() != pk.lower():
        raise ProductError("请先为当前 PK 重新生成数据点，再导出公开样例。")
    outputs = {name: (generated / name).read_text(encoding="utf-8")
               for name in ("datapoints.json", "gizwits_protocol.hpp", "gizwits_protocol.cc")}
    outputs = {name: re.sub(re.escape(pk), "0" * 32, content, flags=re.I) for name, content in outputs.items()}
    if any(ps.lower() in content.lower() for content in outputs.values()):
        raise ProductError("样例中发现 PS，拒绝导出。")
    manifest = dict(format_version=1, product_key_sha256=hashlib.sha256(pk.lower().encode()).hexdigest(),
                    protocol_type=document["protocolType"], datapoint_count=len(document["entities"][0]["attrs"]),
                    files_sha256={name: hashlib.sha256(content.encode()).hexdigest() for name, content in outputs.items()})
    outputs["manifest.json"] = json.dumps(manifest, indent=2) + "\n"
    write_directory(project / "sample", outputs)


def configure(project, sdk, datapoints=None, sample=False):
    main = project / "main"
    generated = main / "generated"
    if project.is_symlink() or main.is_symlink() or generated.is_symlink():
        raise ProductError("工程、main 和 generated 目录不能是符号链接。")
    if not (main / "gizwits_product.cc").is_file():
        raise ProductError("请选择包含 main/gizwits_product.cc 的基础工程。")
    pk, ps = credentials(main / "product_secrets.hpp")
    document = load_sample(project, pk) if sample else load_document(pk, datapoints)
    if ps.lower() in json.dumps(document, ensure_ascii=False).lower():
        raise ProductError("数据点内容包含 PS，请检查输入；PS 仅应保存在本地凭据文件中。")
    spec = importlib.util.spec_from_file_location("gagent_soc_generator", sdk / "tools/generate_soc_board.py")
    generator = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(generator)
    try:
        parsed = generator.normalize(document, pk)
        header, source = generator.codec(*parsed)
    except (ValueError, KeyError, TypeError, AttributeError, IndexError, OverflowError):
        raise ProductError("数据点定义不受支持：请检查协议类型、字段类型、范围及 512 字节报文上限。") from None
    declaration = 'inline constexpr char product_key[] = ' + json.dumps(document["product_key"]) + ';'
    if header.count(declaration) != 1:
        raise ProductError("SDK 生成器格式不兼容，未修改原有文件。")
    header = header.replace(declaration, "inline constexpr char product_key[] = GAGENT_SOC_PRODUCT_KEY;")
    header = header.replace('#include "gagent/protocol/protocol.hpp"',
                            '#include "product_config.hpp"\n#include "gagent/protocol/protocol.hpp"')
    identity = '#pragma once\nnamespace gagent::board::configuration {\ninline constexpr char generated_product_key[] = ' + json.dumps(pk) + ';\n}\n'
    outputs = {"gizwits_protocol.hpp": header, "gizwits_protocol.cc": source,
               "product_identity.hpp": identity,
               "datapoints.json": json.dumps(document, ensure_ascii=False, indent=2) + "\n",
               "generation.json": json.dumps(dict(generator="soc-board-v1", product_key=pk,
                   protocol_type=document["protocolType"], datapoint_count=len(parsed[1])), indent=2) + "\n"}
    # 完整生成成功后才替换目录；失败时还原旧版本。整个 generated 目录由工具管理。
    write_directory(generated, outputs)
    return len(parsed[1]), document["protocolType"]


def main():
    sdk = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project-dir", type=Path, default=sdk.parent, help="独立工程目录，默认为 SDK 的上一级")
    source = parser.add_mutually_exclusive_group()
    source.add_argument("--datapoints", type=Path, help="平台导出的本地 JSON；不访问网络")
    source.add_argument("--from-cache", action="store_true", help="离线使用上次生成时保存的数据点 JSON")
    source.add_argument("--sample", action="store_true", help="离线重建随仓库的默认产品样例，须填写该产品的 PK/PS")
    parser.add_argument("--export-sample", action="store_true", help="生成成功后，将脱敏 JSON 和协议代码导出到可提交的 sample/（替换旧样例）")
    args = parser.parse_args()
    project = args.project_dir.absolute()
    datapoints = project / "main/generated/datapoints.json" if args.from_cache else args.datapoints
    try:
        count, protocol = configure(project, sdk, datapoints, sample=args.sample)
        if args.export_sample:
            export_sample(project)
    except ProductError as error:
        parser.exit(1, str(error) + "\n")
    except (OSError, ValueError, KeyError, TypeError, ImportError):
        parser.exit(1, "生成失败，请检查本地文件、SDK 完整性和目录权限；业务代码不会被覆盖。\n")
    print(f"已生成 {count} 个数据点（{protocol}）：main/generated/。")
    print("PK/PS 及生成目录均为本地文件，请勿提交。业务回调保持不变。下一步：idf.py build")
    if args.export_sample:
        print("公开样例已更新到 sample/，其中不含真实 PK/PS。")


if __name__ == "__main__":
    main()
