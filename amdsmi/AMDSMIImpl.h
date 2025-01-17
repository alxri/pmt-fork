#include <amd_smi/amdsmi.h>

#include "AMDSMI.h"

namespace pmt::amdsmi {

class AMDSMIImpl : public AMDSMI {
 public:
  AMDSMIImpl(const unsigned device_number);
  ~AMDSMIImpl();

 private:
  virtual State GetState() override;

  virtual const char *GetDumpFilename() override {
    return "/tmp/pmt_amdsmi.out";
  }

  amdsmi_processor_handle processor_;
};

}  // end namespace pmt::amdsmi