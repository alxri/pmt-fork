#include <nvml.h>

#include "NVML.h"
#include "common/PMT.h"

namespace nvml {
class Context;
class Device;
}  // end namespace nvml
namespace pmt::nvml {

struct NVMLMeasurement {
  std::string name_;
  unsigned int milliwatt_;
  Timestamp timestamp_;
};

class NVMLState {
 public:
  operator State();
  std::vector<NVMLMeasurement> measurements_;
};

class NVMLImpl : public NVML {
 public:
  NVMLImpl(int device_number);
#if defined(PMT_NVML_CUDAWRAPPERS_API)
  NVMLImpl(cu::Device& device);
#endif

 private:
  void Initialize();
  State GetState() override;

  virtual const char* GetDumpFilename() override { return "/tmp/pmt_nvml.out"; }

  std::vector<NVMLMeasurement> GetMeasurements();

#if not defined(PMT_NVML_LEGACY_MODE)
  const unsigned int kFieldIdPowerInstant = NVML_FI_DEV_POWER_INSTANT;
  const unsigned int kFieldIdPowerAverage = NVML_FI_DEV_POWER_AVERAGE;
  unsigned int nr_scopes_;
#endif

  std::unique_ptr<::nvml::Context> context_;
  std::unique_ptr<::nvml::Device> device_;
};

}  // end namespace pmt::nvml