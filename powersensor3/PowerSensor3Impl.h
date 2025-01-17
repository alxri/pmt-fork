#include <memory>
#include <string_view>

#include <PowerSensor.hpp>

#include "common/PMT.h"
#include "PowerSensor3.h"

namespace pmt::powersensor3 {

class PowerSensor3Impl : public PowerSensor3 {
 public:
  PowerSensor3Impl(const char *device);

 private:
  State GetState() override;

  virtual const char *GetDumpFilename() override {
    return "/tmp/pmt_powersensor3.out";
  }

  std::unique_ptr<::PowerSensor3::PowerSensor> powersensor_;
  std::vector<std::string> pair_names_;
};

}  // end namespace pmt::powersensor3