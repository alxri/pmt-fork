#ifndef PMT_RAPLIMPL_H_
#define PMT_RAPLIMPL_H_

#include <cstddef>
#include <mutex>
#include <string>
#include <vector>

#include "Rapl.h"
#include "RaplCounter.h"
#include "common/PMT.h"

namespace pmt::rapl {

struct RaplMeasurement {
  std::string name;
  std::size_t ujoules;
};

class RaplImpl : public Rapl {
 public:
  RaplImpl();
  ~RaplImpl();

  State GetState() override;

  virtual const char *GetDumpFilename() override { return "/tmp/pmt_rapl.out"; }

 private:
  void Init();
  std::vector<int> fd_energy_uj_;

  std::vector<RaplMeasurement> GetMeasurements();

  std::vector<RaplCounter> rapl_counters_;

  // Mutex used to guard GetMeasurements()
  std::mutex mutex_;
};

}  // end namespace pmt::rapl

#endif  // PMT_RAPLIMPL_H_
