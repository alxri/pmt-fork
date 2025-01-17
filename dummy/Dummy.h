#ifndef PMT_DUMMY_H_
#define PMT_DUMMY_H_

#include <memory>

#include "common/PMT.h"

namespace pmt {
class Dummy : public PMT {
 public:
  static std::unique_ptr<Dummy> Create();
  void UseFixedWatts(const std::vector<float>& watt) { watt_ = watt; }
  void UseFixedJoules(const std::vector<float>& watt) {
    fixed_joules_ = true;
    watt_ = watt;
  }

 protected:
  bool fixed_joules_ = false;
  std::vector<float> watt_;
};
}  // end namespace pmt

#endif  // PMT_DUMMY_H_
