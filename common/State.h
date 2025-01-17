#ifndef PMT_STATE_H_
#define PMT_STATE_H_

#include <cassert>
#include <string>
#include <vector>

#include <pmt/common/Timestamp.h>

namespace pmt {

class State {
 public:
  State &operator=(const State &state) {
    timestamp_ = state.timestamp_;
    nr_measurements_ = state.nr_measurements_;
    name_ = state.name_;
    joules_ = state.joules_;
    watt_ = state.watt_;
    return *this;
  }

  State(int nr_measurements = 1) : nr_measurements_(nr_measurements) {
    name_.resize(nr_measurements);
    joules_.resize(nr_measurements);
    watt_.resize(nr_measurements);
    for (int i = 0; i < nr_measurements; i++) {
      name_[i] = "";
      joules_[i] = 0;
      watt_[i] = 0;
    }
  }

  Timestamp timestamp() const { return timestamp_; }
  int NrMeasurements() const { return nr_measurements_; }
  std::string name(int i) const;
  float joules(int i) const;
  float watts(int i) const;

  Timestamp timestamp_;
  int nr_measurements_;
  std::vector<std::string> name_;
  std::vector<float> joules_;
  std::vector<float> watt_;
};

}  // namespace pmt

#endif  // PMT_STATE_H_