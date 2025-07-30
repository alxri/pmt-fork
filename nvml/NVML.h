#ifndef PMT_NVML_H_
#define PMT_NVML_H_

#include <memory>
#include <string_view>

#if defined(PMT_CUDAWRAPPERS_API)
#include <cudawrappers/cu.hpp>
#endif

#include "common/PMT.h"

namespace pmt::nvml {
class NVML : public PMT {
 public:
  constexpr static inline std::string_view name = "nvml";
  static std::unique_ptr<NVML> Create(int device_number = 0);
#if defined(PMT_CUDAWRAPPERS_API)
  static std::unique_ptr<NVML> Create(cu::Device& device);
#endif
};
}  // end namespace pmt::nvml

#endif  // PMT_NVML_H_
