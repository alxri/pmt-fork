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

  State state(::PowerSensor3::MAX_PAIRS);
  state.name_ = pair_names_;
  state.timestamp_ = measurement.timeAtRead;

  for (size_t i = 0; i < state.nr_measurements_; i++) {
    state.watt_[i] = measurement.voltage[i] * measurement.current[i];
    state.joules_[i] = measurement.consumedEnergy[i];
  }

  return state;
}
}  // end namespace pmt::powersensor3
