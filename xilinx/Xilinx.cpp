#include "Xilinx.h"

#include <fstream>
#include <istream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <errno.h>
#include <ext/alloc_traits.h>
#include <stdlib.h>

#include "common/Exception.h"

namespace {
float GetPower(const std::string &filename) {
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (errno != 0) {
    std::ostringstream message;
    message << "Could not open: " << filename;
    throw pmt::Exception(message.str().c_str());
  }

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
    return "/tmp/pmt_zcu104.out";
  }
};

std::unique_ptr<Xilinx> Xilinx::Create(const char *device) {
  return std::make_unique<XilinxImpl>(device);
}

 XilinxImpl::XilinxImpl(const char *device) {}
//   char *pmt_device = getenv("PMT_DEVICE");
//   filename_ = pmt_device ? pmt_device : device;
// }

State XilinxImpl::GetState() {
  State state;
  state.timestamp_ = GetTime();

  // 1. Read from ZCU104 (in uW, convert to W)
  double core_watts  = ::GetPower("/sys/class/hwmon/hwmon0/power1_input") * 1e-6;
  double aux_watts   = ::GetPower("/sys/class/hwmon/hwmon1/power1_input") * 1e-6;
  double board_watts = ::GetPower("/sys/class/hwmon/hwmon2/power1_input") * 1e-6;

  // 2. CORE+LOGIC
  state.name_.push_back("CORE_LOGIC");
  state.watt_.push_back(core_watts);

  // 3. AUXILIARY (e.g., DRAM, PCIe, etc.)
  state.name_.push_back("AUX");
  state.watt_.push_back(aux_watts);

  // 4. TOTAL FPGA (SUM CORE + AUX)
  state.name_.push_back("FPGA_TOTAL");
  state.watt_.push_back(core_watts + aux_watts);

  // 5. TOTAL FPGA BOARD 
  state.name_.push_back("BOARD");
  state.watt_.push_back(board_watts);

  return state;
}

}  // end namespace pmt::xilinx