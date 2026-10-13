#include <assert.h>
#include <math.h>

#include "../../openquatt/includes/control/oq_heating_curve_duo_floor_logic.h"
namespace {
using namespace oq_curve;

// Regression: in maintain regime HP1 was added while HP2 ran at level 2 (~2.6 kW) and both HPs at their lowest level
// (~3.3 kW) overshot the demand, so HP1 was shed again after the disable hold and restarted ~13 minutes later.
void test_duo_waits_for_lead_to_reach_duo_floor() {
  assert(!duo_floor_reached(2600.0f, 3300.0f));
  assert(!duo_floor_reached(0.0f, 3300.0f));
  assert(duo_floor_reached(3300.0f, 3300.0f));
  assert(duo_floor_reached(4600.0f, 3300.0f));
}

void test_unknown_model_keeps_previous_behaviour() {
  assert(duo_floor_reached(NAN, 3300.0f));
  assert(duo_floor_reached(2600.0f, NAN));
  assert(duo_floor_reached(INFINITY, 3300.0f));
  assert(duo_floor_reached(2600.0f, 0.0f));
}
}  // namespace
int main() {
  test_duo_waits_for_lead_to_reach_duo_floor();
  test_unknown_model_keeps_previous_behaviour();
  return 0;
}
