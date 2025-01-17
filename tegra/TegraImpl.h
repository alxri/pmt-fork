#include <string>
#include <vector>

#include "Tegra.h"
#include "common/PMT.h"

namespace pmt::tegra {

using TegraMeasurement = std::pair<std::string, int>;

class TegraImpl : public Tegra {
 public:
  TegraImpl();
  virtual ~TegraImpl();

  State GetState() override;

  virtual const char *GetDumpFilename() override {
    return "/tmp/pmt_tegra.out";
  }

 private:
  std::vector<TegraMeasurement> GetMeasurements();

  std::string filename_ = "";
  bool started_tegrastats_ = false;
  const int measurement_interval_ = 10;  // milliseconds
};

}  // end namespace pmt::tegra