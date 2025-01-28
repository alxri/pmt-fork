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
  // Initialize all field values
  field_values_.resize(field_names_.size());
  const size_t scope_count = field_names_.size() / field_ids_.size();
  for (int i = 0; i < field_names_.size(); i++) {
    field_values_[i].fieldId = field_ids_[i / scope_count];
    field_values_[i].scopeId = i % scope_count;
  }

  // First call to getFieldValues, some may fail
  device_->getFieldValues(field_names_.size(), field_values_.data());

  // Remove all field values that failed. Remember which index
  // should be reported first.
  for (int i = 0; i < field_names_.size(); i++) {
    if (field_values_[i].nvmlReturn != NVML_SUCCESS) {
      field_names_.erase(field_names_.begin() + i);
      field_values_.erase(field_values_.begin() + i);
      i--;
    } else if (field_names_[i].compare(kDefaultFieldName) == 0) {
      default_field_id_ = i;
    }
  }
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
  std::vector<NVMLMeasurement> measurements;
  device_->getFieldValues(field_values_.size(), field_values_.data());

  if (field_values_[default_field_id_].nvmlReturn != NVML_SUCCESS) {
    std::ostringstream message;
    message << "The default field id '" << kDefaultFieldName
            << "' is not available.";
    throw pmt::Exception(message.str().c_str());
  }

  for (int i = 0; i < field_values_.size(); i++) {
    if (field_values_[i].nvmlReturn == NVML_SUCCESS) {
      measurements.push_back({.name_ = field_names_[i],
                              .milliwatt_ = field_values_[i].value.uiVal,
                              .timestamp_ = Timestamp(std::chrono::microseconds(
                                  field_values_[i].timestamp))});
    }
  }

  if (measurements.empty()) {
    throw pmt::Exception("No measurements available.");
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
  std::swap(state.name_[0], state.name_[default_field_id_]);
  std::swap(state.watt_[0], state.watt_[default_field_id_]);
  state.timestamp_ = measurements[default_field_id_].timestamp_;
#else
  state.timestamp_ = measurements[0].timestamp_;
#endif

  return state;
}
}  // end namespace pmt::nvml
