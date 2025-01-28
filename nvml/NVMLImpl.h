#include <array>

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
  // The following power fields are exposed through NVML. Not all
  // fields are available on all devices and/or software versions.
  std::vector<std::string> field_names_ = {"gpu_average",    "module_average",
                                           "memory_average", "gpu_instant",
                                           "module_instant", "cpu_instant"};

  // Power fields are either average or instantenous
  const std::array<unsigned int, 2> field_ids_ = {NVML_FI_DEV_POWER_AVERAGE,
                                                  NVML_FI_DEV_POWER_INSTANT};

  // This elements of this vector are written by NVML
  std::vector<nvmlFieldValue_t> field_values_;

  // The default field id is set in Initialize() by looking for a measurement
  // with the specified default field name. GetState() will set the value of
  // this field as default (first element).
  unsigned int default_field_id_ = 0;
  const std::string kDefaultFieldName = "gpu_instant";
#endif

  std::unique_ptr<::nvml::Context> context_;
  std::unique_ptr<::nvml::Device> device_;
};

}  // end namespace pmt::nvml