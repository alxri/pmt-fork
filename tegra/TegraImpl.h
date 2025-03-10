#include <vector>

#include "common/PMT.h"
#include "Powermon.h"
#include "Tegra.h"

namespace pmt::tegra {
class TegraImpl : public Tegra {
 public:
  TegraImpl();

  State GetState() override;

  virtual const char *GetDumpFilename() override {
    return "/tmp/pmt_tegra.out";
  }

 private:
  std::vector<Powermon> powermons_;

  const int measurement_interval_ = 10;  // milliseconds
};

}  // end namespace pmt::tegra