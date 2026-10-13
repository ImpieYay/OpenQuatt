#pragma once

#include <math.h>

namespace oq_curve {

// Adding the second HP only helps once the running HP already delivers at least the duo floor (both HPs at their
// lowest level). Below that, duo overshoots the demand and is shed again after the disable hold, causing cycling.
// Unknown model values keep the previous behaviour.
inline bool duo_floor_reached(float lead_applied_power_w, float duo_floor_power_w) {
  if (!isfinite(lead_applied_power_w) || !isfinite(duo_floor_power_w) || duo_floor_power_w <= 0.0f) return true;
  return lead_applied_power_w >= duo_floor_power_w;
}

}  // namespace oq_curve
