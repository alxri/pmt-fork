#ifndef PMT_ROCM_H_
#define PMT_ROCM_H_

#include <memory>
#include <string>

#include "common/PMT.h"

namespace pmt::rocm {
class ROCM : public PMT {
 public:
  inline static std::string name = "rocm";
  static std::unique_ptr<ROCM> Create(int device_number = 0);
};
}  // end namespace pmt::rocm

#endif  // PMT_ROCM_H_
