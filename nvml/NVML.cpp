#include "NVML.h"
#include "NVMLImpl.h"

namespace pmt::nvml {

std::unique_ptr<NVML> NVML::Create(int device_number) {
  return std::unique_ptr<NVML>(new NVMLImpl(device_number));
}

#if defined(PMT_CUDAWRAPPERS_API)
std::unique_ptr<NVML> NVML::Create(cu::Device& device) {
  return std::unique_ptr<NVML>(new NVMLImpl(device));
}
#endif

}  // end namespace pmt::nvml