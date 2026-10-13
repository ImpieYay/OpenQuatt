#include <assert.h>
#include <math.h>

#include "../../openquatt/includes/control/oq_heating_curve_duo_floor_logic.h"
namespace {
using namespace oq_curve;

// Maintain regime at full demand: the lead HP is still ramping (level 2, ~2.6 kW) and the lowest duo combination
// would deliver ~3.3 kW.
DualOnInput maintain_ramping() {
  DualOnInput in;
  in.eligible = true;
  in.duo_better = true;
  in.single_short = true;
  in.demand_high = true;
  in.temperature_error_c = 1.5f;
  in.lead_ramping = true;
  in.lead_power_w = 2600.0f;
  in.duo_floor_power_w = 3300.0f;
  return in;
}

// Regression: HP1 was added while HP2 was still ramping below the duo floor, overshot the demand and was shed again
// after the disable hold, restarting ~13 minutes later.
void test_ramping_lead_below_floor_waits() {
  assert(!dual_hp_on(maintain_ramping()));
  auto reached = maintain_ramping();
  reached.lead_power_w = 3300.0f;
  assert(dual_hp_on(reached));
}

// A lead that cannot step up (frequency cap, excluded range, maximum level) must not block the second HP.
void test_capped_lead_keeps_duo_available() {
  auto capped = maintain_ramping();
  capped.lead_ramping = false;
  assert(dual_hp_on(capped));
}

void test_forced_paths_bypass_floor() {
  auto forced = maintain_ramping();
  forced.forced = true;
  forced.duo_better = false;
  forced.demand_high = false;
  assert(dual_hp_on(forced));
  forced.eligible = false;
  assert(!dual_hp_on(forced));
}

void test_unknown_model_keeps_previous_behaviour() {
  auto in = maintain_ramping();
  in.lead_power_w = NAN;
  assert(dual_hp_on(in));
  in = maintain_ramping();
  in.duo_floor_power_w = NAN;
  assert(dual_hp_on(in));
  in.duo_floor_power_w = 0.0f;
  assert(dual_hp_on(in));
}

void test_existing_conditions_unchanged() {
  auto in = maintain_ramping();
  in.lead_ramping = false;
  in.duo_better = false;
  assert(!dual_hp_on(in));
  in.duo_better = true;
  in.single_short = false;
  assert(!dual_hp_on(in));
  in.single_short = true;
  in.demand_high = false;
  assert(!dual_hp_on(in));
  in.demand_high = true;
  in.temperature_error_c = -0.10f;
  assert(!dual_hp_on(in));
  in.temperature_error_c = NAN;
  assert(dual_hp_on(in));
}
}  // namespace
int main() {
  test_ramping_lead_below_floor_waits();
  test_capped_lead_keeps_duo_available();
  test_forced_paths_bypass_floor();
  test_unknown_model_keeps_previous_behaviour();
  test_existing_conditions_unchanged();
  return 0;
}
