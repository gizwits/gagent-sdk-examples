"""基础工程的开发者入口检查：无需 ESP-IDF、联网或真实产品凭据。"""
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
CHIPS = ("c2", "c3", "c5")
PK, PS = "1" * 32, "2" * 32


def fixture():
    return {"name": "Unit test only", "product_key": PK, "packetVersion": "0x00000004",
            "protocolType": "var_len", "entities": [{"attrs": [
                {"name": "power", "id": 0, "data_type": "bool", "type": "status_writable",
                 "position": {"byte_offset": 0, "bit_offset": 0, "len": 1, "unit": "bit"}}
            ]}]}


class BasicProjects(unittest.TestCase):
    def test_offline_setup_compile_and_reject_stale_product(self):
        compiler = shutil.which("c++")
        if not compiler:
            self.skipTest("C++17 host compiler required")
        for chip in CHIPS:
            with self.subTest(chip=chip), tempfile.TemporaryDirectory() as temporary:
                example = ROOT / "examples" / (chip + "_iot_basic")
                sdk = example / "gagent-sdk"
                project = Path(temporary)
                shutil.copytree(example / "main", project / "main", ignore=shutil.ignore_patterns("generated", "product_secrets.hpp", ".generated-*"))
                secrets = project / "main/product_secrets.hpp"
                secrets.write_text(f'#define GAGENT_SOC_PRODUCT_KEY "{PK}"\n#define GAGENT_SOC_PRODUCT_SECRET "{PS}"\n')
                data = project / "product.local.json"
                data.write_text(json.dumps(fixture()))
                callbacks = (project / "main/gizwits_product.cc").read_bytes()
                command = [sys.executable, str(sdk / "configure_product.py"), "--project-dir", str(project), "--datapoints", str(data)]
                subprocess.run(command, check=True, capture_output=True)
                self.assertEqual(callbacks, (project / "main/gizwits_product.cc").read_bytes())
                compile_command = [compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror", "-fsyntax-only",
                    "-I" + str(project / "main"), "-I" + str(project / "main/generated"),
                    "-I" + str(sdk / "components/gagent_core/include"),
                    str(project / "main/gizwits_product.cc"), str(project / "main/generated/gizwits_protocol.cc")]
                subprocess.run(compile_command, check=True, capture_output=True)
                secrets.write_text(secrets.read_text().replace(PK, "3" * 32))
                result = subprocess.run(compile_command, capture_output=True, text=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("PK changed", result.stderr)
                result = subprocess.run(command, capture_output=True, text=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("PK", result.stderr)
                self.assertNotIn(PS, result.stdout + result.stderr)

    def test_shipped_default_sample_matches_regenerated_protocol(self):
        import hashlib
        for chip in CHIPS:
            with self.subTest(chip=chip), tempfile.TemporaryDirectory() as temporary:
                example = ROOT / "examples" / (chip + "_iot_basic")
                sample = example / "sample"
                manifest = json.loads((sample / "manifest.json").read_text())
                for name, digest in manifest["files_sha256"].items():
                    self.assertEqual(hashlib.sha256((sample / name).read_bytes()).hexdigest(), digest)
                data = json.loads((sample / "datapoints.json").read_text())
                self.assertEqual(data["product_key"], "0" * 32)
                self.assertEqual([a["name"] for a in data["entities"][0]["attrs"]],
                                 ["power", "using_led_light", "power_memory", "timer_s"])
                data["product_key"] = PK
                project = Path(temporary)
                shutil.copytree(example / "main", project / "main", ignore=shutil.ignore_patterns("generated", "product_secrets.hpp"))
                (project / "main/product_secrets.hpp").write_text(f'#define GAGENT_SOC_PRODUCT_KEY "{PK}"\n#define GAGENT_SOC_PRODUCT_SECRET "{PS}"\n')
                document = project / "test.local.json"
                document.write_text(json.dumps(data))
                subprocess.run([sys.executable, str(example / "gagent-sdk/configure_product.py"),
                    "--project-dir", str(project), "--datapoints", str(document)], check=True, capture_output=True)
                for name in ("gizwits_protocol.hpp", "gizwits_protocol.cc"):
                    self.assertEqual((project / "main/generated" / name).read_bytes(), (sample / name).read_bytes())

    def test_shared_partition_layout_and_sdk_hashes(self):
        spec = importlib.util.spec_from_file_location("install_sdk", ROOT / "tools/install_sdk.py")
        installer = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(installer)
        layouts = []
        for chip in CHIPS:
            example = ROOT / "examples" / (chip + "_iot_basic")
            sdk = example / "gagent-sdk"
            installer.verify(sdk, json.loads((example / "sdk.lock.json").read_text()))
            self.assertEqual((example / "partitions.csv").read_bytes(), (sdk / "partitions.csv").read_bytes())
            layouts.append((example / "partitions.csv").read_bytes())
        self.assertTrue(all(layout == layouts[0] for layout in layouts))

    def test_missing_configuration_has_actionable_cmake_error(self):
        cmake = shutil.which("cmake")
        if not cmake:
            self.skipTest("CMake required")
        for chip in CHIPS:
            with self.subTest(chip=chip), tempfile.TemporaryDirectory() as temporary:
                project = Path(temporary) / "project"
                project.mkdir()
                example = ROOT / "examples" / (chip + "_iot_basic")
                shutil.copy2(example / "CMakeLists.txt", project / "CMakeLists.txt")
                (project / "gagent-sdk").mkdir()
                (project / "gagent-sdk/gagent-sdk.cmake").write_text("")
                result = subprocess.run([cmake, "-S", str(project), "-B", str(Path(temporary) / "build")], capture_output=True, text=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("Configure this product first", result.stderr)
                self.assertIn("configure_product.py", result.stderr)


if __name__ == "__main__":
    unittest.main()
