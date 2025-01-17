#include <rocm_smi/rocm_smi.h>

#include "ROCM.h"

namespace pmt::rocm {
class ROCMImpl : public ROCM {
 public:
  ROCMImpl(const unsigned device_number);
  ~ROCMImpl();

 private:
  virtual State GetState() override;

  virtual const char *GetDumpFilename() override { return "/tmp/pmt_rocm.out"; }

  unsigned int device_number_;
};
}  // end namespace pmt::rocm