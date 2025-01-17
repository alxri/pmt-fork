#include <sstream>
#include <vector>

#include "AMDSMIImpl.h"
#include "common/Exception.h"

namespace {
class AmdsmiError : public pmt::Exception {
 public:
  explicit AmdsmiError(amdsmi_status_t result, const char *file, int line)
      : result_(result), file_(file), line_(line) {}

  const char *what() const noexcept override {
    std::ostringstream message;
    message << "AMDSMI call failed with status: " << result_ << " in " << file_
            << " at line " << line_;
    message_ = message.str();
    return message_.c_str();
  }

  operator amdsmi_status_t() const { return result_; }

 private:
  amdsmi_status_t result_;
  const char *file_;
  int line_;
  mutable std::string message_;
};

inline void checkAmdsmiCall(amdsmi_status_t result, const char *file,
                            int line) {
  if (result != AMDSMI_STATUS_SUCCESS) {
    throw AmdsmiError(result, file, line);
  }
}

#define checkAmdsmiCall(call) checkAmdsmiCall((call), __FILE__, __LINE__)

amdsmi_processor_handle Initialize(int device) {
  // Initialize amdsmi
  checkAmdsmiCall(amdsmi_init(AMDSMI_INIT_AMD_GPUS));

  // Get socket count
  uint32_t socket_count = 0;
  checkAmdsmiCall(amdsmi_get_socket_handles(&socket_count, nullptr));

  // Get socket handles
  std::vector<amdsmi_socket_handle> sockets(socket_count);
  checkAmdsmiCall(amdsmi_get_socket_handles(&socket_count, &sockets[0]));

  // Get processor handles for all sockets
  std::vector<amdsmi_processor_handle> processors;

  for (uint32_t i = 0; i < socket_count; i++) {
    // Get the processor count for the socket
    uint32_t processor_count = 0;
    checkAmdsmiCall(
        amdsmi_get_processor_handles(sockets[i], &processor_count, nullptr));

    // Get the processor handle
    amdsmi_processor_handle processor;
    checkAmdsmiCall(
        amdsmi_get_processor_handles(sockets[i], &processor_count, &processor));
    processors.push_back(processor);
  }

  // Select the processor handle to use
  return processors[device];
}

size_t GetPower(amdsmi_processor_handle processor) {
  amdsmi_power_info_t power_measure = {};
  checkAmdsmiCall(amdsmi_get_power_info(processor, &power_measure));
  return power_measure.average_socket_power;
}
}  // namespace

namespace pmt::amdsmi {

AMDSMIImpl::AMDSMIImpl(const unsigned device_number) {
  processor_ = Initialize(device_number);
}

AMDSMIImpl::~AMDSMIImpl() { checkAmdsmiCall(amdsmi_shut_down()); }

State AMDSMIImpl::GetState() {
  State state;
  state.timestamp_ = GetTime();
  state.name_[0] = "device";
  state.watt_[0] = GetPower(processor_);
  return state;
}

}  // end namespace pmt::amdsmi