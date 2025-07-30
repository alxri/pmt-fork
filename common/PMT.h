#ifndef PMT_PMT_H_
#define PMT_PMT_H_

#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <pmt/common/State.h>
#include <pmt/common/Timestamp.h>

namespace pmt {

const std::string kDumpFilenameVariable = "PMT_DUMP_FILE";
const std::string kDumpIntervalVariable = "PMT_DUMP_INTERVAL";

class PMT {
 public:
  ~PMT();

  static double seconds(const Timestamp &timestamp);

  static double seconds(const Timestamp &first, const Timestamp &second);

  static double seconds(const State &first, const State &second);

  static double joules(const State &first, const State &second);

  static double watts(const State &first, const State &second);

  void StartDump(const char *filename = nullptr);
  void StopDump();

  virtual void Mark(const State &state, const std::string &message) const;

  void SetMeasurementInterval(unsigned int milliseconds) {
    measurement_interval_ = milliseconds;
  };
  unsigned int GetMeasurementInterval() const {
    return measurement_interval_;
  };                               // in milliseconds
  unsigned int GetDumpInterval();  // in milliseconds

  State Read();

 protected:
  virtual State GetState() { return state_latest_; };

  virtual const char *GetDumpFilename() = 0;

  void Dump(const State &state);

  Timestamp GetTime();

  unsigned int measurement_interval_ = 1;  // milliseconds

  // The last State returned by Read()
  State state_returned_;

  // The latest State returned by GetState()
  State state_latest_;

 private:
  // Wait up to this number of measurement intervals for the backend to
  // initialize, which it signals by unlocking the thread_mutex_.
  static constexpr int kNrRetriesInitialization = 10;

  // This thread continuously calls GetState to update state_latest_. It is
  // started automatically upon the first Read() call.
  std::thread thread_;
  volatile bool thread_started_ = false;
  volatile bool thread_stop_ = false;
  mutable std::mutex thread_mutex_;

  void StartThread();
  void StopThread();

  void DumpHeader(const State &state);

  std::unique_ptr<std::ofstream> dump_file_ = nullptr;
  mutable std::mutex dump_file_mutex_;
};

std::unique_ptr<PMT> Create(const std::string &name,
                            const std::string &argument = "");

}  // end namespace pmt

std::ostream &operator<<(std::ostream &os, const pmt::Timestamp &timestamp);

#endif  // PMT_PMT_H_
