#include <vector>

#include "Dummy.h"

namespace pmt {

class DummyImpl : public Dummy {
 private:
  virtual State GetState() override;

  virtual const char *GetDumpFilename() override { return nullptr; }
};

std::unique_ptr<Dummy> Dummy::Create() { return std::make_unique<DummyImpl>(); }

// Returns a State object containing:
// - timestamp: current system time
// - name[0]: "total" for accumulated values
// - name[1..n]: "sensor0".."sensorN" for individual sensors
//
// Operates in three modes:
// 1. Default: report zero power and energy (wattt_.size() == 0)
//
// 2. Fixed joules (fixed_joules_ == true):
//    - Accumulates energy based on fixed power values
//    - Skips accumulation on first measurement
//    - joules[0]: sum of all sensors
//    - joules[1..n]: individual sensor values
//
// 3. Fixed watts (fixed_joules_ == false):
//    - Reports constant power values
//    - watt[0]: sum of all sensors
//    - watt[1..n]: individual sensor values
State DummyImpl::GetState() {
  State state(1 + watt_.size());
  state.timestamp_ = PMT::GetTime();
  state.name_[0] = "total";
  if (fixed_joules_) {
    // Accumulate joules as if the device consumed watt_ watts over the previous
    // measurement interval, skipping the first measurement where the previous
    // timestamp is zero.
    if (state_latest_.timestamp_.time_since_epoch().count()) {
      for (size_t i = 0; i < watt_.size(); i++) {
        state.name_[i] = "sensor" + std::to_string(i);
        state.joules_[i + 1] = state_latest_.joules_[i + 1] +
                               watt_[i] * PMT::seconds(state_latest_, state);
        state.joules_[0] += state.joules_[i + 1];
      }
    }
  } else {
    // Use a fixed wattage
    for (size_t i = 0; i < watt_.size(); i++) {
      state.name_[i] = "sensor" + std::to_string(i);
      state.watt_[0] += watt_[i];
      state.watt_[i + 1] = watt_[i];
    }
  }
  return state;
}

}  // end namespace pmt
