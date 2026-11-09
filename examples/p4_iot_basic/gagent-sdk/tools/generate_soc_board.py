#!/usr/bin/env python3
"""Generate a board-owned P0 codec and SoC application from Gizwits datapoints.

Only the Python standard library is required. No network access with --datapoints.
Generated protocol files may be refreshed with --update; user code is preserved.
"""
import argparse
import json
import math
from pathlib import Path
import re
import tempfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
TEMPLATES = Path(__file__).resolve().parent / "templates/soc"
TYPES = ("bool", "enum", "uint8", "uint16", "uint32", "binary")
GROUPS = ("status_writable", "status_readonly", "alert", "fault")


def integer(value, label, low=0, high=4096):
    if type(value) is not int or not low <= value <= high:
        raise ValueError("Invalid " + label)
    return value


def normalize(document, product_key):
    if not re.fullmatch(r"[0-9a-fA-F]{32}", product_key):
        raise ValueError("Product key must contain 32 hexadecimal characters")
    if "entities" not in document and isinstance(document.get("datas"), dict):
        document = document["datas"]
    if document.get("product_key", "").lower() != product_key.lower():
        raise ValueError("Datapoint product key does not match requested product")
    if document.get("protocolType") not in ("standard", "var_len"):
        raise ValueError("Only standard and var_len P0 protocols are supported")
    if document.get("packetVersion") != "0x00000004":
        raise ValueError("Unsupported datapoint packetVersion")
    entities = document.get("entities")
    if not isinstance(entities, list) or len(entities) != 1:
        raise ValueError("Exactly one datapoint entity is supported")
    attrs = entities[0].get("attrs")
    if not isinstance(attrs, list) or not 1 <= len(attrs) <= 256:
        raise ValueError("Expected 1..256 datapoints")
    fields, names, ids = [], set(), set()
    for index, attr in enumerate(attrs):
        name, kind, group = attr.get("name", ""), attr.get("data_type"), attr.get("type")
        # Prefix all emitted C++ members so names such as 'switch' are legal.
        if not re.fullmatch(r"[A-Za-z][A-Za-z0-9_]*", name) or "__" in name or name in names:
            raise ValueError("Invalid or duplicate datapoint name: " + str(name))
        names.add(name)
        ident = integer(attr.get("id"), "datapoint id", high=65535)
        if ident in ids:
            raise ValueError("Duplicate datapoint id")
        ids.add(ident)
        if kind not in TYPES or group not in GROUPS:
            raise ValueError("Unsupported datapoint type: " + name)
        if group in ("alert", "fault") and kind != "bool":
            raise ValueError("Alerts and faults must be boolean")
        pos = attr.get("position", {})
        bit = kind in ("bool", "enum")
        width = integer(pos.get("len"), "field length", 1, 512)
        offset = integer(pos.get("byte_offset"), "byte offset", high=511)
        bit_offset = integer(pos.get("bit_offset"), "bit offset")
        if pos.get("unit") != ("bit" if bit else "byte"):
            raise ValueError("Inconsistent field unit: " + name)
        if bit and (width > 32 or (kind == "bool" and width != 1)):
            raise ValueError("Invalid bit width: " + name)
        if kind.startswith("uint") and width != int(kind[4:]) // 8:
            raise ValueError("Invalid integer width: " + name)
        f = dict(index=index, name=name, kind=kind, group=group, width=width,
                 offset=offset, bit_offset=bit_offset, bit=bit, ratio=1, addition=0,
                 minimum=0, maximum=1, cpp="bool", default="false")
        if kind == "enum":
            values = attr.get("enum")
            if not isinstance(values, list) or not 1 <= len(values) <= 2 ** width:
                raise ValueError("Invalid enum: " + name)
            f.update(maximum=len(values)-1, cpp="std::uint32_t", default="0")
        elif kind.startswith("uint"):
            spec = attr.get("uint_spec", {})
            minimum = integer(spec.get("min"), "minimum", high=2**(width*8)-1)
            maximum = integer(spec.get("max"), "maximum", minimum, 2**(width*8)-1)
            ratio, addition = spec.get("ratio"), spec.get("addition")
            if any(type(v) not in (int, float) or not math.isfinite(v) for v in (ratio, addition)) or ratio <= 0:
                raise ValueError("Finite positive ratio and finite addition required: " + name)
            lo, hi = minimum * ratio + addition, maximum * ratio + addition
            if not all(math.isfinite(v) and abs(v) <= 2**53-1 for v in (lo, hi)):
                raise ValueError("Scaled values exceed exact supported range: " + name)
            integral = float(ratio).is_integer() and float(addition).is_integer()
            cpp = ("std::int64_t" if lo < 0 else "std::uint64_t") if integral else "double"
            f.update(minimum=minimum, maximum=maximum, ratio=ratio, addition=addition,
                     cpp=cpp, default=str(int(lo)) if integral else repr(float(lo)))
        elif kind == "binary":
            f.update(cpp=f"std::array<std::uint8_t, {width}>", default="{}")
        fields.append(f)
    standard = document["protocolType"] == "standard"
    if standard:
        occupied = set()
        group_ends = {}
        for group in GROUPS:
            group_fields = [f for f in fields if f["group"] == group]
            bits = [f for f in group_fields if f["bit"]]
            if bits:
                bases = {f["offset"] for f in bits}
                if len(bases) != 1:
                    raise ValueError("Bit group requires a shared byte_offset")
                base = bases.pop()
                size = (max(f["bit_offset"] + f["width"] for f in bits) + 7)//8
                for f in bits:
                    f.update(segment_base=base, segment_size=size)
                # Entire bit block, including reserved bits, occupies these bytes.
                for byte in range(base, base+size):
                    if byte in occupied:
                        raise ValueError("Overlapping datapoint groups")
                    occupied.add(byte)
                used_bits = set()
                for f in bits:
                    positions = set(range(f["bit_offset"], f["bit_offset"]+f["width"]))
                    if used_bits & positions:
                        raise ValueError("Overlapping bit fields")
                    used_bits |= positions
            for f in group_fields:
                if f["bit"]:
                    end = f["segment_base"] + f["segment_size"]
                else:
                    positions = set(range(f["offset"], f["offset"]+f["width"]))
                    if occupied & positions:
                        raise ValueError("Overlapping byte fields")
                    occupied |= positions
                    end = f["offset"] + f["width"]
                group_ends[group] = max(group_ends.get(group, 0), end)
        # Writable P0 values form the initial fixed block in the legacy standard format.
        control_size = group_ends.get("status_writable", 0)
        for f in fields:
            if f["group"] != "status_writable" and f["offset"] < control_size:
                raise ValueError("Non-writable field overlaps the writable block")
        status_size = max(occupied, default=-1) + 1
    else:
        control_size = 0
        status_size = (sum(f["width"] for f in fields if f["bit"])+7)//8 + sum(f["width"] for f in fields if not f["bit"])
    writable = [f for f in fields if f["group"] == "status_writable"]
    order = writable if standard else sorted(fields, key=lambda f: TYPES.index(f["kind"]))
    for flag, f in enumerate(order):
        f["flag"] = flag
    flags = (len(order)+7)//8
    limit = 1 + status_size + (0 if standard else flags)
    if max(limit, 1+flags+control_size) > 512:
        raise ValueError("Datapoints exceed the generator's 512-byte P0 budget")
    return document, fields, standard, flags, control_size, status_size


def load_document(product_key, path=None, url=None, no_proxy=False):
    if path:
        return json.loads(Path(path).read_text(encoding="utf-8"))
    address = url or "http://site.gizwits.com/v2/datapoint?product_key=" + product_key
    open_url = urllib.request.build_opener(urllib.request.ProxyHandler({})).open if no_proxy else urllib.request.urlopen
    with open_url(address, timeout=20) as response:
        data = response.read(2*1024*1024+1)
    if len(data) > 2*1024*1024:
        raise ValueError("Datapoint response too large")
    return json.loads(data)


def codec(document, fields, standard, flags, control_size, status_size):
    members = "\n".join(f"    {f['cpp']} value_{f['name']} = {f['default']};" for f in fields)
    events = "\n".join(f"    dp_{f['name']} = {f['index']}," for f in fields)
    header = (TEMPLATES/"gizwits_protocol.hpp.in").read_text()
    for key, value in dict(MEMBERS=members, EVENTS=events, NAMES=", ".join(json.dumps(f["name"]) for f in fields), COUNT=len(fields), PK=document["product_key"],
                           MAX_SIZE=1+status_size+(0 if standard else flags), PROTOCOL="00000002",
                           WRITE_ACTION=1 if standard else 0x11, READ_ACTION=2 if standard else 0x12,
                           READ_RESPONSE_ACTION=3 if standard else 0x13, REPORT_ACTION=4 if standard else 0x14).items():
        header = header.replace("@"+key+"@", str(value))
    source = (TEMPLATES/"gizwits_protocol.cc.in").read_text()
    decode, encode, validate, changes = [], [], [], []
    order = fields if standard else sorted(fields, key=lambda f: TYPES.index(f["kind"]))
    flag_read = [f"    mask[{f['index']}] = get_bits(input.data, {flags}, {f['flag']}, 1) != 0;" for f in fields if "flag" in f]
    if standard:
        flag_read += [f"    if (get_bits(input.data, {flags}, {i}, 1)) return Status::malformed;" for i in range(sum(f['group']=='status_writable' for f in fields),flags*8)]
    else:
        flag_read += [f"    if (get_bits(input.data, {flags}, {i}, 1)) return Status::malformed;" for i in range(len(fields),flags*8)]
    for f in order:
        idx, name, kind = f["index"], "value_"+f["name"], f["kind"]
        if kind == "binary":
            check = ""
        elif kind.startswith("uint"):
            check = f"    if (!raw_value(double(value.{name}), {f['ratio']}, {f['addition']}, {f['minimum']}u, {f['maximum']}u, raw)) return false;"
        else:
            check = f"    if (value.{name} > {f['maximum']}u) return false;" if kind == "enum" else ""
        if check:
            validate.append(check)
        changes.append(f"    mask[{idx}] = before.{name} != after.{name};")
        sel = "true" if standard else f"mask[{idx}]"
        if f["bit"]:
            read = f"get_bits(values + {f['segment_base']}, {f['segment_size']}, {f['bit_offset']}, {f['width']})" if standard else f"get_bits(values, bit_bytes, bit_offset, {f['width']})"
            write = f"put_bits(values + {f['segment_base']}, {f['segment_size']}, {f['bit_offset']}, {f['width']}, std::uint32_t(value.{name}));" if standard else f"put_bits(values, bit_bytes, bit_offset, {f['width']}, std::uint32_t(value.{name})); bit_offset += {f['width']};"
            decoded = f"        next.{name} = static_cast<{f['cpp']}>({read});"
            if not standard:
                decoded += f"\n        bit_offset += {f['width']};"
        else:
            offset = str(f["offset"]) if standard else "byte_offset"
            if kind == "binary":
                decoded = f"        std::memcpy(next.{name}.data(), values + {offset}, {f['width']});"
                write = f"std::memcpy(values + {offset}, value.{name}.data(), {f['width']});"
            else:
                lower = f"raw < {f['minimum']}u || " if f['minimum'] else ""
                decoded = f"        const auto raw = get_uint(values + {offset}, {f['width']});\n        if ({lower}raw > {f['maximum']}u) return Status::malformed;\n        next.{name} = static_cast<{f['cpp']}>(double(raw) * {f['ratio']} + {f['addition']});"
                write = f"if (!raw_value(double(value.{name}), {f['ratio']}, {f['addition']}, {f['minimum']}u, {f['maximum']}u, raw)) return Status::malformed;\n        put_uint(values + {offset}, {f['width']}, raw);"
            if not standard:
                decoded += f"\n        byte_offset += {f['width']};"
                write += f" byte_offset += {f['width']};"
        if f["group"] == "status_writable":
            decode.append(f"    if (mask[{idx}]) {{\n{decoded}\n    }}")
        elif not standard:
            decode.append(f"    if (mask[{idx}]) return Status::malformed;")
        encode.append(f"    if ({sel}) {{\n        {write}\n    }}")
    if standard:
        decode_setup = f"    if (input.size != {flags+control_size}) return Status::malformed;\n    [[maybe_unused]] const auto* values = input.data + {flags};"
        encode_setup = f"    (void)mask;\n    constexpr std::size_t size = {status_size};\n    if (!output || capacity < size) return Status::too_large;\n    std::memset(output, 0, size);\n    auto* values = output;"
        query = "    if (input.size != 1) return Status::malformed;\n    const auto mask = all_fields();"
    else:
        sizecode = "\n".join(f"    if (mask[{f['index']}]) {'bit_count' if f['bit'] else 'byte_count'} += {f['width']};" for f in fields)
        decode_setup = "    std::size_t bit_count = 0, byte_count = 0;\n"+sizecode+f"\n    const auto bit_bytes = (bit_count+7)/8;\n    if (input.size != {flags}+bit_bytes+byte_count) return Status::malformed;\n    [[maybe_unused]] const auto* values = input.data + {flags};\n    std::size_t bit_offset = 0, byte_offset = bit_bytes;\n    (void)bit_offset; (void)byte_offset;"
        encode_setup = "    std::size_t bit_count = 0, byte_count = 0;\n"+sizecode+f"\n    const auto bit_bytes = (bit_count+7)/8;\n    const auto size = {flags}+bit_bytes+byte_count;\n    if (!output || capacity < size) return Status::too_large;\n    std::memset(output, 0, size);\n"+"\n".join(f"    put_bits(output, {flags}, {f['flag']}, 1, mask[{f['index']}]);" for f in fields)+f"\n    auto* values = output + {flags};\n    std::size_t bit_offset = 0, byte_offset = bit_bytes;\n    (void)bit_offset; (void)byte_offset;"
        query = f"""    if (input.size != 1+{flags}) return Status::malformed;
    // 全量查询将整个字段标志数组填为 FF，包括末尾填充位。
    bool full = true;
    for (std::size_t i=1; i<input.size; ++i) full = full && input.data[i] == 0xff;
    Fields mask{{}};
    if (full) mask = all_fields();
    else {{
        const auto status = read_flags({{input.data+1, input.size-1}}, mask);
        if (status != Status::ok) return status;
    }}"""
    replacements = dict(FLAGS=flags, READ_FLAGS="\n".join(flag_read), VALIDATE="\n".join(validate),
                        DECODE_SETUP=decode_setup, DECODE="\n".join(decode), ENCODE_SETUP=encode_setup,
                        ENCODE="\n".join(encode), CHANGES="\n".join(changes), QUERY=query)
    for key, value in replacements.items():
        source = source.replace("@"+key+"@", str(value))
    return header, source


def generate(product_key, name=None, target="esp32c3", root=ROOT, document=None, update=False, hardware_board=None):
    name = name or "devkit_" + target
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_-]*", name):
        raise ValueError("Invalid board name")
    if target not in ("esp32c2", "esp32c3", "esp32c5", "esp32s3", "esp32p4"):
        raise ValueError("Unsupported target")
    hardware = None
    if target == "esp32p4":
        if hardware_board is None:
            raise ValueError("P4 requires --hardware-board with explicit Hosted wiring and partitions")
        hardware_board = Path(hardware_board)
        hardware = json.loads(hardware_board.read_text())
        if hardware.get("target") != target:
            raise ValueError("Hardware board target mismatch")
        from p4_config import resolve_hosted
        resolve_hosted(hardware.get("p4_hosted"), target, None, {})
        partition = (hardware_board.parent / hardware["partitions"]).resolve()
        if not partition.is_relative_to(hardware_board.parent.resolve()):
            raise ValueError("Hardware partitions must stay inside the board")
        partition_text = partition.read_text()
    parsed = normalize(document, product_key)
    document = parsed[0]
    destination = root/"boards/soc"/name
    if destination.is_symlink() or not destination.resolve().is_relative_to((root/"boards/soc").resolve()):
        raise ValueError("Board must stay inside boards/soc")
    if destination.exists():
        if not update:
            raise ValueError("Board already exists; use --update to refresh generated files only")
        manifest = json.loads((destination/"generation.json").read_text())
        if manifest.get("product_key") != product_key or manifest.get("generator") != "soc-board-v1":
            raise ValueError("Existing board belongs to a different product/generator")
    header, source = codec(*parsed)
    generated = {"gizwits_protocol.hpp": header, "gizwits_protocol.cc": source,
                 "datapoints.json": json.dumps(document, ensure_ascii=False, indent=2)+"\n",
                 "generation.json": json.dumps(dict(generator="soc-board-v1", product_key=product_key,
                    protocol_type=document["protocolType"], datapoint_count=len(parsed[1])), indent=2)+"\n"}
    scaffold = {filename: (TEMPLATES/(filename+".in")).read_text().replace("@BOARD@", name).replace("@BLUETOOTH@", "GAgent-"+name[:24])
                for filename in ("board.cc", "gizwits_product.hpp", "gizwits_product.cc", "product_config.hpp", "README.md")}
    scaffold["board.json"] = json.dumps(dict(mode="soc", target=target, flash_size="4M", storage_writable=True,
        sources=["gizwits_protocol.cc", "gizwits_product.cc"]), indent=2)+"\n"
    if hardware:
        config = json.loads(scaffold["board.json"])
        config.update(flash_size=hardware["flash_size"], partitions="partitions.csv",
                      p4_hosted=hardware["p4_hosted"], mcu_ota=False, camera={"enabled": False})
        scaffold["board.json"] = json.dumps(config, indent=2)+"\n"
        scaffold["partitions.csv"] = partition_text
        scaffold["board.cc"] = scaffold["board.cc"].replace('"4M"', json.dumps(hardware["flash_size"]))
    scaffold[".gitignore"] = "product_secrets.hpp\n"
    # Validate every target before any update; never follow a user-created output symlink.
    for filename in (*generated, *scaffold):
        if (destination/filename).is_symlink():
            raise ValueError("Refusing symlink output: " + filename)
    destination.mkdir(parents=True, exist_ok=True)
    for filename, contents in {**scaffold, **generated}.items():
        path = destination/filename
        if filename in scaffold and path.exists():
            continue
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", dir=destination, delete=False) as stream:
            stream.write(contents)
            temporary = Path(stream.name)
        temporary.replace(path)
    return destination


def update_project(product_key, destination, document):
    """更新已有独立工程的协议文件；业务源码由例程仓库或客户维护。"""
    destination = Path(destination)
    main = destination / "main"
    if destination.is_symlink() or main.is_symlink():
        raise ValueError("Refusing symlink project")
    manifest = json.loads((main / "generation.json").read_text())
    if manifest.get("product_key") != product_key or manifest.get("generator") != "soc-board-v1":
        raise ValueError("Existing project belongs to a different product/generator")
    parsed = normalize(document, product_key)
    header, source = codec(*parsed)
    generated = {"gizwits_protocol.hpp": header, "gizwits_protocol.cc": source,
                 "datapoints.json": json.dumps(parsed[0], ensure_ascii=False, indent=2)+"\n",
                 "generation.json": json.dumps(dict(manifest, protocol_type=parsed[0]["protocolType"], datapoint_count=len(parsed[1])), indent=2)+"\n"}
    if any((main / name).is_symlink() for name in generated):
        raise ValueError("Refusing symlink generated file")
    for name, content in generated.items():
        (main / name).write_text(content)
    return destination


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--product-key", "--pk", required=True)
    parser.add_argument("--board", help="Product board name; defaults to devkit_<target>")
    parser.add_argument("--target", default="esp32c3", choices=("esp32c2", "esp32c3", "esp32c5", "esp32s3", "esp32p4"))
    parser.add_argument("--hardware-board", type=Path, help="P4 board.json providing Hosted wiring and partition defaults")
    parser.add_argument("--project-dir", "--template-dir", dest="project_dir", type=Path,
                        help="Update an existing independent project (requires --update); clone the examples repository to create one")
    parser.add_argument("--datapoints", type=Path, help="Offline JSON; skips the API")
    parser.add_argument("--url", help="Override datapoint API URL")
    parser.add_argument("--no-proxy", action="store_true", help="Fetch directly; ignore environment/system proxies")
    parser.add_argument("--output-root", type=Path, default=ROOT, help="Project root (default: this checkout); writes boards/soc/<board>")
    parser.add_argument("--update", action="store_true", help="Refresh generated codec/JSON; preserve board and business code")
    args = parser.parse_args()
    try:
        if not re.fullmatch(r"[0-9a-fA-F]{32}", args.product_key):
            raise ValueError("Invalid product key")
        document = load_document(args.product_key, args.datapoints, args.url, args.no_proxy)
        if args.project_dir:
            if not args.update:
                raise ValueError("--project-dir requires --update; create projects from the independent examples repository")
            path = update_project(args.product_key, args.project_dir, document)
        else:
            path = generate(args.product_key, args.board, args.target, root=args.output_root, document=document, update=args.update, hardware_board=args.hardware_board)
    except (ValueError, KeyError, TypeError, OSError) as error:
        parser.exit(1, "SoC generation failed: " + str(error) + "\n")
    print("Generated", path)
    print("Configure product_config.hpp and product callbacks, then build with:")
    print("idf.py build" if args.project_dir else f"python3 tools/build_firmware.py --mode soc --board {path.name} --build-dir /tmp/gagent-{path.name}")


if __name__ == "__main__":
    main()
