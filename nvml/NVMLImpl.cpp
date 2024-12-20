#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <cudawrappers/nvml.hpp>

#include <ext/alloc_traits.h>

#include "NVMLImpl.h"

namespace pmt::nvml {

NVMLState::operator State() {
  State state(measurements_.size());
  state.timestamp_ = timestamp_;
  state.joules_[0] = joules_;
  for (size_t i = 0; i < measurements_.size(); i++) {
    state.name_[i] = measurements_[i].name_;
    state.watt_[i] = measurements_[i].milliwatt_ / 1.e3;
  }
  return state;
}

NVMLImpl::NVMLImpl(int device_number) {
  const char *pmt_device = getenv("PMT_DEVICE");
  device_number = pmt_device ? atoi(pmt_device) : device_number;

  // Initialize CUDA
  cu::init();
  cu::Device device(device_number);

  // Initialize NVML
  context_ = std::make_unique<::nvml::Context>();
  device_ = std::make_unique<::nvml::Device>(*context_, device);

  // Check whether the CPU+GPU scope is supported (e.g. Grace Hopper)
#if not defined(PMT_NVML_LEGACY_MODE)
  nvmlFieldValue_t values[1];
  values[0].fieldId = kFieldIdPowerAverage;
  values[0].scopeId = 1;
  device_->getFieldValues(1, values);
  nr_scopes_ = 1 + (values[0].nvmlReturn == NVML_SUCCESS);
#endif

  // Initialize the first timestamp
  state_previous_.timestamp_ = GetTime();
}

NVMLImpl::~NVMLImpl() { stopped_ = true; }

#if defined(PMT_NVML_LEGACY_MODE)
std::vector<NVMLMeasurement> NVMLImpl::GetMeasurements() {
  return {{.name_ = "gpu_average",
           .milliwatt_ = device_->getPower(),
           .timestamp_ = GetTime()}};
}
#else
std::vector<NVMLMeasurement> NVMLImpl::GetMeasurements() {
  const int nr_field_ids = 2;
  const int nr_measurements = nr_scopes_ * nr_field_ids;
  nvmlFieldValue_t values[nr_measurements];
  const unsigned int field_ids[] = {kFieldIdPowerInstant, kFieldIdPowerAverage};

  std::vector<NVMLMeasurement> measurements(nr_measurements);

  for (int i = 0; i < nr_measurements; i += nr_field_ids) {
    const unsigned int scopeId = i / nr_field_ids;
    values[i].fieldId = field_ids[0];
    values[i].scopeId = scopeId;
    values[i + 1].fieldId = field_ids[1];
    values[i + 1].scopeId = scopeId;
  }

  device_->getFieldValues(nr_measurements, values);

  const std::string scopeNames[] = {"gpu", "module"};
  const std::string suffixes[] = {"_instant", "_average"};

  for (int i = 0; i < nr_scopes_; ++i) {
    for (int j = 0; j < nr_field_ids; ++j) {
      int idx = nr_field_ids * i + j;
      measurements[idx].name_ = scopeNames[i] + suffixes[j];
      measurements[idx].milliwatt_ = values[idx].value.uiVal;
      measurements[idx].timestamp_ =
          Timestamp(std::chrono::microseconds(values[idx].timestamp));
    }
  }

  return measurements;
}
#endif

NVMLState NVMLImpl::GetNVMLState() {
  if (stopped_) {
    return state_previous_;
  }

  NVMLState state;
  try {
    state.measurements_ = GetMeasurements();

    // Default: use the instantaneous GPU power
    // Grace Hopper: use the instantaneous module power
#if defined(PMT_NVML_LEGACY_MODE)
    state.milliwatt_ = state.measurements_[0].milliwatt_;
    state.timestamp_ = state.measurements_[0].timestamp_;
#else
    const unsigned int measurement_id = nr_scopes_ == 1 ? 0 : 2;
    state.milliwatt_ = state.measurements_[measurement_id].milliwatt_;
    state.timestamp_ = state.measurements_[measurement_id].timestamp_;
#endif

    // Set derived fields of state
    const double duration =
        seconds(state_previous_.timestamp_, state.timestamp_);

    if (state_previous_.joules_ > 0) {
      const double watt =
          (state.milliwatt_ + state_previous_.milliwatt_) / 2.e3;
      state.joules_ = state_previous_.joules_ + watt * duration;
    } else {
      const double watt = state.milliwatt_ / 1.e3;
      state.joules_ = watt * duration;
    }

    state_previous_ = state;
  } catch (std::runtime_error &e) {
    return state_previous_;
  }

  return state;
}
}  // end namespace pmt::nvml
