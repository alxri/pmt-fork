#include "State.h"

namespace pmt {
std::string State::name(int i) const {
  assert(i < nr_measurements_);
  return name_[i];
}

float State::joules(int i) const {
  assert(i < nr_measurements_);
  return joules_[i];
}

float State::watts(int i) const {
  assert(i < nr_measurements_);
  return watt_[i];
}
}  // end namespace pmt