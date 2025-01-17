#include "Xilinx.h"

#include <istream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <errno.h>
#include <ext/alloc_traits.h>
#include <stdlib.h>

#include "common/Exception.h"

namespace {
float GetPower(std::string &filename) {
  // Open power file, e.g.
  // /sys/devices/pci0000:a0/0000:a0:03.1/0000:a1:00.0/hwmon/hwmon3/power1_input
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (errno != 0) {
    std::ostringstream message;
    message << "Could not open: " << filename;
    throw pmt::Exception(message.str().c_str());
  }

  // This file has one line with instantenous power consumption in uW
  size_t power;
  file >> power;
  return power;
}

}  // namespace
namespace pmt::xilinx {

class XilinxImpl : public Xilinx {
 public:
  XilinxImpl(const char *device);

 private:
  State GetState() override;

  virtual const char *GetDumpFilename() override {
    return "/tmp/pmt_xilinx.out";
  }

  std::string filename_;
};

std::unique_ptr<Xilinx> Xilinx::Create(const char *device) {
  return std::make_unique<XilinxImpl>(device);
}

XilinxImpl::XilinxImpl(const char *device) {
  char *pmt_device = getenv("PMT_DEVICE");
  filename_ = pmt_device ? pmt_device : device;
}

State XilinxImpl::GetState() {
  State state;
  state.timestamp_ = GetTime();
  state.name_[0] = "device";
  state.watt_[0] = ::GetPower(filename_) * 1e-6;
  return state;
}

}  // end namespace pmt::xilinx
