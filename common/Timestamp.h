#ifndef PMT_TIMESTAMP_H_
#define PMT_TIMESTAMP_H_

#include <chrono>

namespace pmt {

using Timestamp = std::chrono::high_resolution_clock::time_point;

}  // end namespace pmt

#endif  // PMT_TIMESTAMP_H_
