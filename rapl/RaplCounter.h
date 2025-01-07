#ifndef PMT_RAPLCOUNTER_H_
#define PMT_RAPLCOUNTER_H_

#include <charconv>
#include <cstddef>
#include <fstream>
#include <string>

namespace pmt::rapl {

class RaplCounter {
 public:
  RaplCounter(const std::string& directory);

  const std::string& GetName() const { return name_; };

  // The numbers in the rapl /energy_uj files range from zero up to a maximum
  // specified in /max_energy_range_uj. This class reports monotonically
  // increasing values starting with zero for the first measurement. Therefore,
  // the result is computed as follows:
  //  now = <read value>
  //  offset += now < previous ? max : 0
  //  result += offset + now - first
  std::size_t Read();

 private:
  std::ifstream ifstream_energy_uj_;
  std::string name_;
  std::size_t max_energy_range_uj_;
  std::size_t energy_uj_first_ = 0;
  std::size_t energy_uj_previous_ = 0;
  std::size_t energy_uj_offset_ = 0;
};

}  // namespace pmt::rapl

#endif  // PMT_RAPLCOUNTER_H_