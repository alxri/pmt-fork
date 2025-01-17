#include <sstream>

#include <rocm-core/rocm_version.h>

#include "ROCMImpl.h"

namespace {

class RocmError : public std::exception {
 public:
  explicit RocmError(rsmi_status_t result, const char *file, int line)
      : result_(result), file_(file), line_(line) {}

  const char *what() const noexcept override {
    std::ostringstream message;
    message << "RSMI call failed with status: " << result_ << " in " << file_
            << " at line " << line_;
    return message.str().c_str();
  }

  operator rsmi_status_t() const { return result_; }

 private:
  rsmi_status_t result_;
  const char *file_;
  int line_;
};

inline void checkRsmiCall(rsmi_status_t result, const char *file, int line) {
  if (result != RSMI_STATUS_SUCCESS) {
    throw RocmError(result, file, line);
  }
}

#define checkRsmiCall(call) checkRsmiCall((call), __FILE__, __LINE__)

size_t GetPower(uint32_t device_number) {
  uint64_t power;  // in microWatts (uW)
#if ROCM_VERSION_MAJOR < 6
  const uint32_t sensor_number = 0;
  checkRsmiCall(rsmi_dev_power_ave_get(device_number, sensor_number, &power));
#else
  RSMI_POWER_TYPE power_type;
  checkRsmiCall(rsmi_dev_power_get(device_number, &power, &power_type));
#endif

  return static_cast<double>(power) * 1e-6;  // in W
}
}  // namespace

namespace pmt::rocm {

ROCMImpl::ROCMImpl(const unsigned device_number) {
  checkRsmiCall(rsmi_init(0));

  device_number_ = device_number;
}

ROCMImpl::~ROCMImpl() { checkRsmiCall(rsmi_shut_down()); }

State ROCMImpl::GetState() {
  State state;
  state.timestamp_ = GetTime();
  state.name_[0] = "device";
  state.watt_[0] = GetPower(device_number_);
  return state;
}

}  // end namespace pmt::rocm
