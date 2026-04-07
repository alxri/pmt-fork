#include "PowerSensor3Impl.h"

namespace pmt::powersensor3 {

PowerSensor3Impl::PowerSensor3Impl(const char *device)
    : powersensor_(std::make_unique<::PowerSensor3::PowerSensor>(device)) {
  for (unsigned pair_id = 0; pair_id < ::PowerSensor3::MAX_PAIRS; pair_id++) {
    pair_names_.push_back(powersensor_->getPairName(pair_id));
  }
}

State PowerSensor3Impl::GetState() {
  const ::PowerSensor3::State measurement = powersensor_->read();

  State state(1 + ::PowerSensor3::MAX_PAIRS);
  state.name_[0] = "total";
  state.timestamp_ = measurement.timeAtRead;

  for (size_t i = 0; i < pair_names_.size(); i++) {
    state.name_[i + 1] = pair_names_[i];
    state.watt_[i + 1] = measurement.voltage[i] * measurement.current[i];
    state.joules_[i + 1] = measurement.consumedEnergy[i];
    state.watt_[0] += state.watt_[i + 1];
    state.joules_[0] += state.joules_[i + 1];
  }

  return state;
}
}  // end namespace pmt::powersensor3
