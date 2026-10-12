#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "openquatt/includes/odu/oq_odu_compressor_levels.h"
#include "tests/fixtures/v2_new_amh6_eeprom.h"

// The generated include contains the actual YAML callbacks. Only ESPHome I/O
// is replaced here; identity decisions, parsers, tokens and reloads are production code.
#define ESP_LOGW(...) ((void)0)
#define ESP_LOGI(...) ((void)0)

namespace esphome::modbus {
enum class EntityType { HOLDING };
}
namespace oq_energy {
enum class HpInputPowerStatus { UNKNOWN_VARIANT, INVALID_OR_STALE };
}

struct Unit;
void boot(Unit* state);
void offline(Unit* state);
void online(Unit* state);
void detect(Unit* state);
void detect_timeout(Unit* state);
void load(Unit* state);
void load_timeout(Unit* state);

uint32_t millis() { return 1000U; }
struct Sensor {
  template <typename T>
  void publish_state(T) {}
  void invalidate_state() {}
};
struct Value {
  void invalidate() {}
};
struct Sources {
  Value working_mode, compressor_frequency, status_2108, defrost, status_2119;
  Value water_in, water_out, flow, outside;
};
struct Bus {
  void record_offline_transition() {}
};
struct Dump {
  bool is_active() const { return false; }
};
struct Settings {
  void set_odu_identity(uint16_t, oq_odu::Variant) {}
  void notify_odu_offline() {}
};
struct Defrost {
  void set_odu_identity(oq_odu::Variant, uint16_t = 0U) {}
  void offline() {}
};
struct Frequency {
  bool blocked{false};
  void set_extended_layout(bool) {}
  void reset_runtime_state(const char* = nullptr) {}
  bool runtime_reload_blocked_after_write() const { return blocked; }
};
struct Script {
  std::function<void()> action;
  void execute() { action(); }
};
struct Unit {
  uint8_t index;
  oq_odu::CoreIdentity validated_core_identity{};
  int generation_variant_code{0};
  bool odu_generation_detection_complete{false}, odu_generation_request_pending{false};
  bool odu_generation_revalidation_required{true}, runtime_frequency_revalidation_required{true};
  uint32_t odu_generation_request_token{0}, compressor_level_profile_request_token{0};
  bool compressor_level_profile_request_pending{false}, recovery_probe_pending{false}, is_online{true};
  uint32_t runtime_frequency_retry_ms{0}, poll_cycle{0};
  uint32_t voltage_last_update_ms{0}, current_last_update_ms{0}, fan_speed_last_update_ms{0};
  uint32_t status_2108_last_update_ms{0}, pump_feedback_last_update_ms{0};
  int power_input_status_code{0};
  oq_odu::RuntimeFrequencySnapshotStorage runtime_frequency_snapshot_storage{};
  Sensor generation, generation_variant, customer_model_code, pcb_firmware_raw, control_board_item;
  Sensor compressor_level_profile, pump_relay, status_2108_raw, status_2115_raw, pump_ipwm_feedback_raw;
  Dump odu_eeprom_dump;
  Settings settings;
  Defrost defrost;
  Frequency frequency;
  Settings* odu_settings{&settings};
  Defrost* odu_defrost{&defrost};
  Frequency* odu_runtime_frequency{&frequency};
  Script load_runtime_frequency_table_once;
  Sources sources;
  Bus bus;

  explicit Unit(uint8_t index) : index(index) {
    load_runtime_frequency_table_once.action = [this]() { load(this); };
  }
};
struct Manager {
  std::array<bool, 3> valid{};
  std::vector<std::pair<uint8_t, bool>> observations;
  void observe_runtime_frequency_mapping(uint8_t index, bool value, uint32_t) {
    valid.at(index) = value;
    observations.emplace_back(index, value);
  }
  void observe_transport(uint8_t, bool, uint32_t) {}
} manager;
struct {
  bool state{false};
} polling_paused;

using Reply = std::function<void(esphome::modbus::EntityType, uint16_t, std::span<const uint8_t>)>;
struct Request {
  Unit* owner;
  uint16_t address;
  Reply callback;
  void answer(const std::vector<uint8_t>& data) const { callback(esphome::modbus::EntityType::HOLDING, address, data); }
};
std::vector<Request> requests;
namespace openquatt_modbus_shim {
void queue_modbus_read(Unit& unit, esphome::modbus::EntityType, uint16_t address, uint16_t, Reply callback) {
  requests.push_back({&unit, address, std::move(callback)});
}
}  // namespace openquatt_modbus_shim

#include "odu_identity_callbacks.inc"

Request take(Unit& unit, uint16_t address) {
  for (auto it = requests.begin(); it != requests.end(); ++it) {
    if (it->owner == &unit && it->address == address) {
      Request request = *it;
      requests.erase(it);
      return request;
    }
  }
  assert(false && "Expected production Modbus read missing");
  return {};
}
std::vector<uint8_t> words(const uint16_t* source, size_t count) {
  std::vector<uint8_t> data;
  for (size_t i = 0; i < count; ++i) {
    data.push_back(static_cast<uint8_t>(source[i] >> 8U));
    data.push_back(static_cast<uint8_t>(source[i]));
  }
  return data;
}
oq_odu::CoreIdentity identity() {
  return {true, oq_odu::COMPRESSOR_CODE_V2, 0x0201U, oq_odu::CONTROL_BOARD_ITEM_V2_NEW_MODEL};
}
std::vector<uint8_t> core_data(const oq_odu::CoreIdentity& core) {
  std::array<uint16_t, oq_odu::CORE_IDENTITY_REGISTER_COUNT> data{};
  data[oq_odu::COMPRESSOR_CODE_WORD_OFFSET] = core.compressor_code;
  data[oq_odu::PCB_PROGRAM_WORD_OFFSET] = core.pcb_program;
  data[oq_odu::CONTROL_BOARD_ITEM_WORD_OFFSET] = core.control_board_item;
  return words(data.data(), data.size());
}
std::vector<uint8_t> base_data() { return words(oq_test::v2_new_amh6::EEPROM_WORDS.data() + 1, 22); }
std::vector<uint8_t> extension_data() { return words(oq_test::v2_new_amh6::EEPROM_WORDS.data() + 51, 20); }
void read_identity(Unit& unit, const oq_odu::CoreIdentity& core) {
  detect(&unit);
  take(unit, oq_odu::CORE_IDENTITY_REGISTER).answer(core_data(core));
}
void finish_tables(Unit& unit) {
  take(unit, oq_odu::BASE_FREQUENCY_TABLE_REGISTER).answer(base_data());
  take(unit, oq_odu::EXTENDED_FREQUENCY_TABLE_REGISTER).answer(extension_data());
}
void prime(Unit& unit) {
  boot(&unit);
  read_identity(unit, identity());
  finish_tables(unit);
  assert(manager.valid[unit.index]);
  assert(unit.validated_core_identity.available);
}
void initial_and_unchanged() {
  Unit unit(1);
  boot(&unit);
  assert(!unit.validated_core_identity.available);
  read_identity(unit, identity());
  assert(!manager.valid[1]);
  assert(manager.observations.empty());
  finish_tables(unit);
  const auto snapshot = unit.runtime_frequency_snapshot_storage;
  read_identity(unit, identity());
  take(unit, oq_odu::BASE_FREQUENCY_TABLE_REGISTER).answer({0});
  assert(manager.valid[1]);
  assert(unit.runtime_frequency_snapshot_storage == snapshot);
  assert(manager.observations.size() == 1);
}
void program_changed() {
  Unit unit(1);
  prime(unit);
  const auto snapshot = unit.runtime_frequency_snapshot_storage;
  auto revised = identity();
  ++revised.pcb_program;  // Synthetic metadata transition within the same variant.
  read_identity(unit, revised);
  assert(unit.validated_core_identity.pcb_program == revised.pcb_program);
  assert(!manager.valid[1]);
  assert(unit.runtime_frequency_snapshot_storage == snapshot);
  take(unit, oq_odu::BASE_FREQUENCY_TABLE_REGISTER).answer(base_data());
  take(unit, oq_odu::EXTENDED_FREQUENCY_TABLE_REGISTER).answer({0});
  assert(!manager.valid[1]);
  assert(unit.runtime_frequency_snapshot_storage == snapshot);
  load(&unit);
  const auto late = take(unit, oq_odu::BASE_FREQUENCY_TABLE_REGISTER);
  load_timeout(&unit);
  late.answer(base_data());
  assert(!manager.valid[1] && requests.empty());
  load(&unit);
  finish_tables(unit);
  assert(manager.valid[1]);
}
void compressor_changed() {
  Unit unit(1);
  prime(unit);
  auto revised = identity();
  ++revised.compressor_code;
  read_identity(unit, revised);
  assert(!manager.valid[1]);
  assert(unit.validated_core_identity.compressor_code == revised.compressor_code);
  finish_tables(unit);
  assert(manager.valid[1]);
}
void incomplete_unknown() {
  Unit unit(1);
  prime(unit);
  detect(&unit);
  take(unit, oq_odu::CORE_IDENTITY_REGISTER).answer({0});
  assert(manager.valid[1]);
  assert(unit.validated_core_identity.pcb_program == identity().pcb_program);
  auto unsupported = identity();
  unsupported.control_board_item = 0xFFFFU;
  read_identity(unit, unsupported);
  assert(!manager.valid[1]);
  assert(!unit.odu_generation_detection_complete);
  assert(unit.validated_core_identity.control_board_item == identity().control_board_item);
  read_identity(unit, identity());
  assert(!manager.valid[1]);
  finish_tables(unit);
  assert(manager.valid[1]);
}
void transport_flap() {
  Unit unit(1);
  prime(unit);
  const auto snapshot = unit.runtime_frequency_snapshot_storage;
  detect(&unit);
  const auto late = take(unit, oq_odu::CORE_IDENTITY_REGISTER);
  offline(&unit);
  assert(manager.valid[1]);
  assert(unit.validated_core_identity.available);
  assert(unit.runtime_frequency_snapshot_storage == snapshot);
  online(&unit);
  read_identity(unit, identity());
  auto revised = identity();
  ++revised.pcb_program;
  late.answer(core_data(revised));
  assert(unit.validated_core_identity.pcb_program == identity().pcb_program);
  assert(manager.valid[1]);
  finish_tables(unit);
  assert(manager.valid[1]);
}
void write_guard_and_stale_extension() {
  Unit unit(1);
  prime(unit);
  const auto snapshot = unit.runtime_frequency_snapshot_storage;
  load(&unit);
  take(unit, oq_odu::BASE_FREQUENCY_TABLE_REGISTER).answer(base_data());
  const auto late = take(unit, oq_odu::EXTENDED_FREQUENCY_TABLE_REGISTER);
  unit.frequency.blocked = true;
  auto revised = identity();
  ++revised.pcb_program;
  read_identity(unit, revised);
  assert(!manager.valid[1]);
  assert(unit.frequency.blocked);
  late.answer(extension_data());
  assert(!manager.valid[1]);
  assert(requests.empty());
  assert(unit.runtime_frequency_snapshot_storage == snapshot);
}
void duo() {
  Unit first(1), second(2);
  prime(first);
  prime(second);
  const auto second_snapshot = second.runtime_frequency_snapshot_storage;
  load(&second);
  auto revised = identity();
  ++revised.pcb_program;
  read_identity(first, revised);
  assert(!manager.valid[1] && manager.valid[2]);
  assert(second.validated_core_identity.pcb_program == identity().pcb_program);
  finish_tables(second);
  assert(!manager.valid[1] && manager.valid[2]);
  assert(second.runtime_frequency_snapshot_storage == second_snapshot);
  finish_tables(first);
  assert(manager.valid[1] && manager.valid[2]);
}
int main(int argc, char** argv) {
  assert(argc == 2);
  const std::pair<const char*, void (*)()> cases[] = {
      {"initial_and_unchanged", initial_and_unchanged},
      {"program_changed", program_changed},
      {"compressor_changed", compressor_changed},
      {"incomplete_unknown", incomplete_unknown},
      {"transport_flap", transport_flap},
      {"write_guard_and_stale_extension", write_guard_and_stale_extension},
      {"duo", duo},
  };
  for (const auto& [name, run] : cases) {
    if (name == std::string(argv[1])) {
      run();
      return 0;
    }
  }
  assert(false && "Unknown lifecycle scenario");
}
