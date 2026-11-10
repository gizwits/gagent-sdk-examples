"""Select a shipped P4 radio ABI and generate board-owned Hosted defaults."""
import argparse
import json
from pathlib import Path
import sys

CHIPS = ("esp32c5", "esp32c6", "esp32c61")
COMPONENTS = ("gagent_wireless", "gagent_p4")


def select(manifest, chip=None, board=None):
    variants = manifest.get("radio_variants", {})
    board_chip = board.get("p4_hosted", {}).get("chip") if board else None
    if chip and board_chip and chip != board_chip:
        raise ValueError("SDK radio selection disagrees with board.json")
    chip = chip or board_chip or manifest.get("default_radio")
    if chip not in CHIPS or chip not in variants:
        raise ValueError("SDK radio archive unavailable: " + str(chip) + "; available: " + ", ".join(sorted(variants)))
    return chip, variants[chip]


def board_values(sdk, variant, board):
    if not board:
        return {}
    # The exported package and the producer checkout share this exact validator.
    sys.path.insert(0, str(sdk / "tools"))
    from p4_config import resolve_hosted, sdk_values
    raw = board.get("p4_hosted")
    hosted = resolve_hosted(raw, board.get("target"), board.get("application"), board.get("aispeech", {}))
    if not hosted or hosted["chip"] != variant["hosted"]["chip"]:
        raise ValueError("SDK Hosted chip mismatch")
    if hosted["transport"] != variant["hosted"]["transport"]:
        raise ValueError("SDK radio transport mismatch; export an adapter for this transport")
    # Preserve camera pin ownership even though this step only generates radio config.
    camera = board.get("camera", {})
    camera_pins = {v for k, v in camera.items() if k.endswith("_pin") and type(v) is int and v >= 0}
    from p4_config import reserved
    if camera.get("enabled") and camera_pins & reserved(hosted):
        raise ValueError("Hosted GPIO conflicts with camera")
    return sdk_values({"p4_hosted": hosted})


def configured_defaults(sdk, variant, board):
    config = (sdk / variant["sdkconfig"]).read_text()
    overrides = board_values(sdk, variant, board)
    manifest=json.loads((sdk/'sdk-manifest.json').read_text())
    camera_api=manifest.get('camera_config_abi')
    if camera_api:
        from camera import camera_config,sensor_values,sensor_key
        overrides.update(sensor_values(sdk,camera_config(sdk,manifest,board)))
    if not overrides:
        return config
    def key(line):
        return line.split("=", 1)[0] if line.startswith("CONFIG_") else line[2:].split(" ", 1)[0] if line.startswith("# CONFIG_") else ""
    lines = [line for line in config.splitlines() if key(line) not in overrides and not (camera_api and sensor_key(key(line)))]
    lines.extend(f"{name}={value}" if value != "n" else f"# {name} is not set" for name, value in overrides.items())
    return "\n".join(lines) + "\n"


def configure(sdk, output, chip=None, board_path=None):
    manifest = json.loads((sdk / "sdk-manifest.json").read_text())
    board = json.loads(board_path.read_text()) if board_path else None
    chip, variant = select(manifest, chip, board)
    config = configured_defaults(sdk, variant, board)
    output.mkdir(parents=True, exist_ok=True)
    defaults = output / "sdk-radio.defaults"
    # Avoid needlessly invalidating ESP-IDF's config cache.
    if not defaults.exists() or defaults.read_text() != config:
        defaults.write_text(config)
    cmake = output / "sdk-radio.cmake"
    binding_cmake=''
    if manifest.get('board_config_abi'):
        from camera import generate_binding
        binding=generate_binding(sdk,output,manifest,board)
        binding_cmake='list(APPEND EXTRA_COMPONENT_DIRS "'+str(binding)+'")\n'
    cmake.write_text(binding_cmake + f'set(GAGENT_SDK_RADIO "{chip}")\nset(GAGENT_SDK_RADIO_DEFAULTS "{defaults.as_posix()}")\n')


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--chip", choices=CHIPS)
    parser.add_argument("--board", type=Path)
    args = parser.parse_args()
    configure(args.sdk, args.output, args.chip, args.board)
