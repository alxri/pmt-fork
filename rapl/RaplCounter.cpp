#include <cassert>
#include <fstream>

#include "RaplCounter.h"
namespace pmt::rapl {

RaplCounter::RaplCounter(const std::string& directory) {
  std::ifstream ifstream_directory(directory + "/name");
  assert(ifstream_directory.is_open());
  ifstream_directory >> name_;

  std::ifstream ifstream_max_energy_range_uj(directory +
                                             "/max_energy_range_uj");
  assert(ifstream_max_energy_range_uj.is_open());
  ifstream_max_energy_range_uj >> max_energy_range_uj_;

  ifstream_energy_uj_ = std::ifstream(directory + "/energy_uj");
  assert(ifstream_energy_uj_.is_open());

  const std::size_t energy_uj = Read();
  energy_uj_first_ = energy_uj;
  energy_uj_previous_ = energy_uj;
  energy_uj_offset_ = 0;
}

std::size_t RaplCounter::Read() {
  std::size_t energy_uj;
  assert(ifstream_energy_uj_.is_open());
  ifstream_energy_uj_ >> energy_uj;
  ifstream_energy_uj_.seekg(0);
  energy_uj_offset_ +=
      energy_uj < energy_uj_previous_ ? max_energy_range_uj_ : 0;
  return energy_uj_offset_ + energy_uj - energy_uj_first_;
}

}  // end namespace pmt::rapl