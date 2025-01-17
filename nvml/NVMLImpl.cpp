#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <cudawrappers/nvml.hpp>

#include <ext/alloc_traits.h>

#include "NVMLImpl.h"
#include "common/Exception.h"

namespace pmt::nvml {

NVMLImpl::NVMLImpl(int device_number) {
  const char *pmt_device = getenv("PMT_DEVICE");
  device_number = pmt_device ? atoi(pmt_device) : device_number;

  // Initialize CUDA
  cu::init();
  cu::Device device(device_number);

  // Initialize NVML
  context_ = std::make_unique<::nvml::Context>();
  device_ = std::make_unique<::nvml::Device>(device);

  Initialize();
}

#if defined(PMT_NVML_CUDAWRAPPERS_API)
NVMLImpl::NVMLImpl(cu::Device &device)
    : device_(std::make_unique<::nvml::Device>(device)) {
  Initialize();
}
#endif

void NVMLImpl::Initialize() {
  // Check whether the CPU+GPU scope is supported (e.g. Grace Hopper)
#if not defined(PMT_NVML_LEGACY_MODE)
  nvmlFieldValue_t values[1];
  values[0].fieldId = kFieldIdPowerAverage;
  values[0].scopeId = 1;
  device_->getFieldValues(1, values);
  nr_scopes_ = 1 + (values[0].nvmlReturn == NVML_SUCCESS);
#endif
}

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

State NVMLImpl::GetState() {
  std::vector<NVMLMeasurement> measurements;
  try {
    measurements = GetMeasurements();
  } catch (const ::nvml::Error &e) {
    throw pmt::Exception(e.what());
  }

  State state(measurements.size());

  for (size_t i = 0; i < measurements.size(); i++) {
    state.name_[i] = measurements[i].name_;
    state.joules_[i] = 0;
    state.watt_[i] = measurements[i].milliwatt_ / 1.e3;
  }

#if !defined(PMT_NVML_LEGACY_MODE)
  // Default: use the instantaneous GPU power
  // Grace Hopper: use the instantaneous module power
  const unsigned int measurement_id = nr_scopes_ == 1 ? 0 : 2;
  std::swap(state.name_[0], state.name_[measurement_id]);
  std::swap(state.watt_[0], state.watt_[measurement_id]);
  state.timestamp_ = measurements[measurement_id].timestamp_;
#else
  state.timestamp_ = measurements[0].timestamp_;
#endif

  return state;
}
}  // end namespace pmt::nvml
