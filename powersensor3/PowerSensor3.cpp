#include <vector>

#include <ext/alloc_traits.h>

#include "PowerSensor3.h"

#include <PowerSensor.hpp>

const unsigned MAX_PAIRS = PowerSensor3::MAX_PAIRS;

namespace {
double Seconds(const PowerSensor3::State &first,
               const PowerSensor3::State &second) {
  return PowerSensor3::seconds(first, second);
}

double Joules(const PowerSensor3::State &first,
              const PowerSensor3::State &second, int pair_id) {
  return PowerSensor3::Joules(first, second, pair_id);
}

double Watt(const PowerSensor3::State &first, const PowerSensor3::State &second,
            int pair_id) {
  return PowerSensor3::Watt(first, second, pair_id);
}
}  // namespace

namespace pmt::powersensor3 {

template <typename PowerSensor, typename PowerSensorState>
class PowerSensor3Impl : public PowerSensor3 {
 public:
  PowerSensor3Impl(const char *device)
      : powersensor_(std::make_unique<PowerSensor>(device)),
        first_state_(powersensor_->read()) {
    for (unsigned pair_id; pair_id < MAX_PAIRS; pair_id++) {
      pairNames[pair_id] = GetPairName(pair_id);
    }
  }

  State GetState() override {
    State state(MAX_PAIRS + 1);
    const PowerSensorState powersensor_state = powersensor_->read();

    state.name_[0] = "total";
    state.timestamp_ = ::Seconds(first_state_, powersensor_state);
    state.joules_[0] = ::Joules(first_state_, powersensor_state, -1);
    state.watt_[0] = ::Watt(first_state_, powersensor_state, -1);

    for (unsigned pair_id; pair_id < MAX_PAIRS; pair_id++) {
      state.name_[pair_id + 1] = pairNames[pair_id];
      state.joules_[pair_id + 1] =
          ::Joules(first_state_, powersensor_state, pair_id);
      state.watt_[pair_id + 1] =
          ::Watt(first_state_, powersensor_state, pair_id);
    }
    return state;
  }

  State Read(int pair_id) override {
    State state{1};
    State current_state{MAX_PAIRS + 1};
    current_state = PMT::Read();

    state.timestamp_ = current_state.timestamp_;
    state.name_[0] = current_state.name_[pair_id + 1];
    state.joules_[0] = current_state.joules_[pair_id + 1];
    state.watt_[0] = current_state.watt_[pair_id + 1];
    return state;
  }

  bool IsInUse(int pair_id) override { return powersensor_->getInUse(pair_id); }

  std::string GetPairName(int pair_id) override {
    return powersensor_->getPairName(pair_id);
  }

 private:
  virtual const char *GetDumpFilename() override {
    return "/tmp/pmt_powersensor3.out";
  }

  std::string pairNames[MAX_PAIRS];
  std::unique_ptr<PowerSensor> powersensor_{};
  PowerSensorState first_state_{};
};

std::unique_ptr<PowerSensor3> PowerSensor3::Create(const char *device) {
  return std::make_unique<
      PowerSensor3Impl<::PowerSensor3::PowerSensor, ::PowerSensor3::State>>(
      device);
}

}  // end namespace pmt::powersensor3
