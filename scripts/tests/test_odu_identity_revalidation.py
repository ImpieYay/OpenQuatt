"""Compile and exercise the production YAML callbacks with asynchronous Modbus reads."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import textwrap
import unittest


ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / "openquatt/oq_HP_io.yaml").read_text()


def callbacks(start, end):
    block = SOURCE[SOURCE.index(start):SOURCE.index(end, SOURCE.index(start))]
    lines = block.splitlines()
    result = []
    for index, line in enumerate(lines):
        if line.strip() != "- lambda: |-":
            continue
        indent = len(line) - len(line.lstrip())
        body = []
        for candidate in lines[index + 1:]:
            if candidate.strip() and len(candidate) - len(candidate.lstrip()) <= indent:
                break
            body.append(candidate)
        code = textwrap.dedent("\n".join(body))
        code = re.sub(r"id\(\$\{hp_id\}_(\w+)\)", r"state->\1", code)
        code = code.replace("id(${hp_id})", "*state")
        code = code.replace("oq_sources::${hp_id}.", "state->sources.")
        code = code.replace("id(mod_bus)", "state->bus")
        code = code.replace("id(oq_incident_manager)", "manager")
        code = code.replace("id(oq_runtime_polling_paused)", "polling_paused")
        code = code.replace("${hp_index}", "state->index").replace("${prefix}", "test")
        if "${" in code or "id(" in code:
            raise AssertionError("Unmapped production callback dependency")
        result.append(code)
    return result


class OduIdentityRevalidationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = shutil.which("g++") or shutil.which("clang++")
        if compiler is None:
            raise unittest.SkipTest("C++ compiler unavailable")
        cls.directory = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.directory.cleanup)
        build = Path(cls.directory.name)
        boot, = callbacks("esphome:\n", "# MODBUS CONTROLLER")
        offline, = callbacks("on_offline:", "on_online:")
        online, = callbacks("on_online:", "# Slow readback controller")
        detect, detect_timeout = callbacks("id: ${hp_id}_detect_odu_generation_once",
                                           "id: ${hp_id}_load_runtime_frequency_table_once")
        load, load_timeout = callbacks("id: ${hp_id}_load_runtime_frequency_table_once",
                                       "id: ${hp_id}_detect_odu_generation\n")
        generated = "\n".join(f"void {name}(Unit* state) {{\n{body}\n}}"
                              for name, body in (("boot", boot), ("offline", offline), ("online", online),
                                                 ("detect", detect), ("detect_timeout", detect_timeout),
                                                 ("load", load), ("load_timeout", load_timeout)))
        (build / "odu_identity_callbacks.inc").write_text(generated)
        cls.binary = build / "odu_identity_revalidation"
        compiled = subprocess.run([compiler, "-std=c++20", "-Wall", "-Wextra", "-Werror",
                                   "-I", str(ROOT), "-I", str(build),
                                   str(ROOT / "scripts/tests/fixtures/odu_identity_revalidation.cpp"),
                                   "-o", str(cls.binary)], capture_output=True, text=True)
        if compiled.returncode != 0:
            raise AssertionError(compiled.stdout + compiled.stderr)

    def scenario(self, name):
        result = subprocess.run([str(self.binary), name], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_initial_and_unchanged_identity(self):
        self.scenario("initial_and_unchanged")

    def test_program_change_requires_complete_reload(self):
        self.scenario("program_changed")

    def test_compressor_change_requires_reload(self):
        self.scenario("compressor_changed")

    def test_incomplete_and_unknown_identity(self):
        self.scenario("incomplete_unknown")

    def test_transport_flap_keeps_confirmed_snapshot(self):
        self.scenario("transport_flap")

    def test_write_guard_and_stale_table_callback(self):
        self.scenario("write_guard_and_stale_extension")

    def test_duo_identity_changes_are_independent(self):
        self.scenario("duo")


if __name__ == "__main__":
    unittest.main()
