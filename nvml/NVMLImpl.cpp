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

State NVMLImpl::GetState() {
  try {
#if defined(PMT_NVML_LEGACY_MODE)
    State state;
    state.name_[0] = "gpu_average";
    state.watt_[0] = device_->getPower() / 1.e3;
    state.timestamp_ = GetTime();
    return state;
#else
    // Take a measurement
    device_->getFieldValues(field_values_.size(), field_values_.data());

    // Initialize the state
    State state(field_names_.size());
    state.timestamp_ = Timestamp(
        std::chrono::microseconds(field_values_[default_field_id_].timestamp));
    for (int i = 0; i < state.NrMeasurements(); i++) {
      state.name_[i] = field_names_[i];
      state.watt_[i] = field_values_[i].value.uiVal / 1.e3;
    }

    // Set the default field first
    std::swap(state.name_[0], state.name_[default_field_id_]);
    std::swap(state.watt_[0], state.watt_[default_field_id_]);

    return state;
#endif
  } catch (const ::nvml::Error &e) {
    throw pmt::Exception(e.what());
  }
}
}  // end namespace pmt::nvml
