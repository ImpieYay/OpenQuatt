#pragma once

#include <math.h>

namespace oq_curve {

struct DualOnInput {
  bool eligible = false;      // demand active, a duo candidate exists, no startup grace or demand rundown
  bool forced = false;        // emergency capacity or dual defrost
  bool duo_better = false;    // duo matches the power target clearly better than single
  bool single_short = false;  // recovery: single saturated; maintain: single not sufficient
  bool demand_high = false;   // dispatch demand at or above the duo enable threshold
  float temperature_error_c = NAN;
  bool lead_ramping = false;      // lead HP is still below its requested level and its next level is valid
  float lead_power_w = NAN;       // predicted output of the lead HP at its applied level
  float duo_floor_power_w = NAN;  // lowest predicted output of any valid duo combination
};

// While the lead HP is still ramping, adding the second HP below the duo floor overshoots the demand and the second
// HP is shed again after the disable hold, causing cycling. Once the lead cannot step up any further (frequency cap,
// excluded range, maximum level) or the model is unavailable, the previous behaviour applies.
inline bool duo_floor_reached(bool lead_ramping, float lead_power_w, float duo_floor_power_w) {
  if (!lead_ramping || !isfinite(lead_power_w) || !isfinite(duo_floor_power_w) || duo_floor_power_w <= 0.0f) {
    return true;
  }
  return lead_power_w >= duo_floor_power_w;
}

inline bool dual_hp_on(const DualOnInput& in) {
  if (!in.eligible) return false;
  if (in.forced) return true;
  return in.duo_better && in.single_short && in.demand_high &&
         (!isfinite(in.temperature_error_c) || in.temperature_error_c >= -0.05f) &&
         duo_floor_reached(in.lead_ramping, in.lead_power_w, in.duo_floor_power_w);
}

}  // namespace oq_curve
