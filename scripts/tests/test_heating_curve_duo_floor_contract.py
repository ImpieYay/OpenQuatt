import re
import unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
RUNTIME = (ROOT / "openquatt/includes/control/oq_heating_curve_runtime.h").read_text()
class HeatingCurveDuoFloorContractTest(unittest.TestCase):
    def test_dual_on_uses_tested_predicate_with_floor_inputs(self) -> None:
        match = re.search(r"const bool dual_on = oq_curve::dual_hp_on\(\s*\{(.*?)\}\);", RUNTIME, re.S)
        self.assertIsNotNone(match)
        self.assertIn("forced_capacity || dual_defrost", match.group(1))
        floor_args = r"temperature_error_c,\s*lead_ramping,\s*lead_ramping \? level_power_w\(lead_hp1, lead_level\) : NAN,\s*duo_floor_w$"
        self.assertRegex(match.group(1).strip(), floor_args)
        self.assertIn("duo_floor_w = std::isfinite(duo_floor_w) ? std::min(duo_floor_w, item.power_w)", RUNTIME)
        self.assertIn("lead_request > lead_level", RUNTIME)
if __name__ == "__main__":
    unittest.main()
