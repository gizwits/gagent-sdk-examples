"""Reject incompatible toolchains/configuration before linking binary GAgent components."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
from radio import select as select_radio, board_values


def check_partitions(partition, idf, config, manifest, app_bin=None):
    """Use the pinned IDF parser; enforce storage semantics rather than offsets."""
    spec = importlib.util.spec_from_file_location("sdk_partitions", Path(idf) / "components/partition_table/gen_esp32part.py")
    parser = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(parser)
    parser.offset_part_table = int(config.get("CONFIG_PARTITION_TABLE_OFFSET", "0x8000"), 0)
    table = parser.PartitionTable.from_csv(Path(partition).read_text())
    table.verify()
    flash = json.loads(config["CONFIG_ESPTOOLPY_FLASHSIZE"])
    match = re.fullmatch(r"(\d+)MB", flash)
    if not match:
        raise ValueError("Invalid SDK consumer Flash capacity")
    table.verify_size_fits(int(match[1]) * 1024 * 1024)
    def require(name, kind, subtype, minimum):
        p = table.find_by_name(name)
        if p is None or p.type != kind or p.subtype != subtype or p.size < minimum or p.size % 4096:
            raise ValueError("SDK required partition missing or invalid: " + name)
        return p
    require("nvs", 1, 2, 0x3000)
    profile = manifest["board"]["profile"]
    label = manifest["board"].get("configuration_partition")
    for name in ((label,) if label else ("GData_0", "GData_1")):
        require(name, 1, 2, 0x4000 if label else 0x1000)
    if profile.get("mcu_ota"):
        require(manifest["board"].get("mcu_ota_partition", "Reserved"), 1, 2, 0x1000)
    if profile.get("module_ota", True):
        require("otadata", 1, 0, 0x2000)
        for index in range(2):
            p = next(table.find_by_type(0, 0x10 + index), None)
            if p is None or p.size < 0x10000:
                raise ValueError("SDK module OTA requires two application slots")
            if app_bin and Path(app_bin).stat().st_size > p.size:
                raise ValueError("SDK application does not fit every OTA slot")
    if config.get("CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH") == "y":
        p = next(table.find_by_type(1, 3), None)
        if p is None or p.size < 0x10000:
            raise ValueError("SDK coredump requires a data/coredump partition of at least 64 KiB")


def values(path):
    result = {}
    for line in path.read_text().splitlines():
        if line.startswith("CONFIG_") and "=" in line:
            key, value = line.split("=", 1)
            result[key] = value
        else:
            match = re.fullmatch(r"# (CONFIG_\w+) is not set", line)
            if match:
                result[match[1]] = "n"
    return result


def check(sdk, config, idf, compiler, target, app_bin=None, board=None, radio=None):
    manifest = json.loads((sdk / "sdk-manifest.json").read_text())
    for name, digest in manifest["files_sha256"].items():
        if hashlib.sha256((sdk / name).read_bytes()).hexdigest() != digest:
            raise ValueError("SDK file changed: " + name)
    if target != manifest["target"]:
        raise ValueError("SDK target mismatch")
    selected = json.loads(Path(board).read_text()) if board else None
    actual = values(config) if manifest.get("radio_variants") else None
    variant = None
    if manifest.get("radio_variants"):
        from radio import CHIPS
        detected = [chip for chip in CHIPS if actual.get("CONFIG_SLAVE_IDF_TARGET_" + chip.upper()) == "y"]
        if len(detected) != 1:
            raise ValueError("SDK requires exactly one Wi-Fi Remote slave target")
        if radio and radio != detected[0]:
            raise ValueError("SDK radio/config mismatch; use a fresh build and sdkconfig for the selected radio")
        _, variant = select_radio(manifest, detected[0], selected)
    if board:
        if selected.get("target") != target or selected.get("mode") != manifest["board"]["profile"]["mode"]:
            raise ValueError("SDK consumer board mode/target mismatch")
        if variant is None and selected.get("p4_hosted") != manifest["board"].get("p4_hosted"):
            raise ValueError("SDK Hosted wiring mismatch; use libraries built for this wiring")
        camera = manifest["board"].get("camera", {"enabled": False})
        if bool(selected.get("camera", {}).get("enabled")) != bool(camera.get("enabled")):
            raise ValueError("Camera capability differs from the SDK libraries")
        if not manifest.get("camera_config_abi") and camera.get("enabled") and selected.get("camera") != camera:
            raise ValueError("Camera configuration differs from the prebuilt SDK")
    revision = subprocess.check_output(["git", "-C", str(idf), "rev-parse", "HEAD"], text=True).strip()
    if revision != manifest["idf_revision"]:
        raise ValueError("SDK requires ESP-IDF commit " + manifest["idf_revision"])
    version = subprocess.check_output([compiler, "-dumpfullversion"], text=True).strip()
    if version != manifest["compiler_version"]:
        raise ValueError("SDK compiler version mismatch")
    expected, actual = values(sdk / (variant["sdkconfig"] if variant else "sdkconfig.defaults")), values(config)
    if variant:
        expected.update(board_values(sdk, variant, selected))
    if manifest.get("camera_config_abi"):
        from camera import camera_config,sensor_values,sensor_key
        camera_values=sensor_values(sdk,camera_config(sdk,manifest,selected))
        expected={k:v for k,v in expected.items() if not sensor_key(k)}
        expected.update(camera_values)
    # File location is relocatable; compare partition contents instead.
    paths = {"CONFIG_PARTITION_TABLE_FILENAME", "CONFIG_PARTITION_TABLE_CUSTOM_FILENAME"}
    if manifest.get("format_version", 1) >= 2:
        # Capacity affects the image header and layout, not the library ABI.
        paths.update(k for k in expected if re.fullmatch(r"CONFIG_ESPTOOLPY_FLASHSIZE(?:_\d+MB)?", k))
    bundle_key = "CONFIG_MBEDTLS_CUSTOM_CERTIFICATE_BUNDLE_PATH"
    custom_bundle = expected.get("CONFIG_MBEDTLS_CUSTOM_CERTIFICATE_BUNDLE") == "y"
    if custom_bundle:
        paths.add(bundle_key)
    mismatch = sorted(k for k, v in expected.items() if k not in paths and actual.get(k, "n") != v)
    if mismatch:
        raise ValueError("SDK config mismatch: " + ", ".join(mismatch[:20]))
    partition = Path(actual["CONFIG_PARTITION_TABLE_CUSTOM_FILENAME"].strip('"'))
    if manifest.get("format_version", 1) >= 2:
        check_partitions(partition, idf, actual, manifest, app_bin)
    elif partition.read_bytes() != (sdk / "partitions.csv").read_bytes():
        raise ValueError("SDK partition layout mismatch")
    if custom_bundle:
        bundle = Path(json.loads(actual.get(bundle_key, '""')))
        if not bundle.is_file() or bundle.read_bytes() != (sdk / "certs/custom-bundle.pem").read_bytes():
            raise ValueError("SDK custom certificate bundle mismatch")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("sdk", "sdkconfig", "idf"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--target", required=True)
    parser.add_argument("--app-bin", type=Path)
    parser.add_argument("--board", type=Path)
    parser.add_argument("--radio")
    args = parser.parse_args()
    check(args.sdk, args.sdkconfig, args.idf, args.compiler, args.target, args.app_bin, args.board, args.radio)
