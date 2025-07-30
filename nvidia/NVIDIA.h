#ifndef PMT_NVIDIA_H_
#define PMT_NVIDIA_H_

#include <memory>
#include <string_view>

#if defined(PMT_CUDAWRAPPERS_API)
#include <cudawrappers/cu.hpp>
#endif

#include "common/PMT.h"

namespace pmt::nvidia {
class NVIDIA : public PMT {
 public:
  constexpr static inline std::string_view name = "nvidia";
  static std::unique_ptr<PMT> Create(int device_number = 0);
#if defined(PMT_CUDAWRAPPERS_API)
  static std::unique_ptr<PMT> Create(cu::Device& device);
#endif
};
}  // end namespace pmt::nvidia

#endif  // PMT_NVIDIA_H_
