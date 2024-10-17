#ifndef PMT_AMDSMI_H_
#define PMT_AMDSMI_H_

#include <memory>
#include <string>

#include "common/PMT.h"

namespace pmt::amdsmi {
class AMDSMI : public PMT {
 public:
  inline static std::string name = "amdsmi";
  static std::unique_ptr<AMDSMI> Create(int device_number = 0);
};
}  // end namespace pmt::amdsmi

#endif  // PMT_AMDSMI_H_
