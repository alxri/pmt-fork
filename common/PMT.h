#ifndef PMT_COMMON_H_
#define PMT_COMMON_H_

#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace pmt {

const std::string kDumpFilenameVariable = "PMT_DUMP_FILE";
const std::string kDumpIntervalVariable = "PMT_DUMP_INTERVAL";

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
  }

  int NrMeasurements() { return nr_measurements_; }

  double timestamp() { return timestamp_; }
  std::string name(int i);
  float joules(int i);
  float watts(int i);

  double timestamp_;
  int nr_measurements_;
  std::vector<std::string> name_;
  std::vector<float> joules_;
  std::vector<float> watt_;
};

class PMT {
 public:
  virtual ~PMT();

  static double seconds(const State &first, const State &second);

  static double joules(const State &first, const State &second);

  static double watts(const State &first, const State &second);

  void StartDump(const char *filename = nullptr);
  void StopDump();

  virtual void Mark(const State &start, const State &current,
                    const std::string &message) const;

  virtual int GetMeasurementInterval() = 0;  // in milliseconds
  virtual float GetDumpInterval();           // in seconds

  State Read();

 protected:
  virtual State GetState() { return state_previous_; };

  virtual const char *GetDumpFilename() = 0;

  void Dump(const State &start, const State &second);

  static double GetTime();

 private:
  // The last state returned by Read()
  State state_previous_;

  // The last state set by the thread
  State state_latest_;

  // This thread continuously call GetState to update state_latest_. It is
  // started automatically upon the first Read() call.
  std::thread thread_;
  volatile bool thread_started_ = false;
  volatile bool thread_stop_ = false;

  void StartThread();
  void StopThread();

  void DumpHeader(const State &state);

 protected:
  std::unique_ptr<std::ofstream> dump_file_ = nullptr;
  mutable std::mutex dump_file_mutex_;
};

std::unique_ptr<PMT> Create(const std::string &name, int argument);
std::unique_ptr<PMT> Create(const std::string &name,
                            const char *argument = nullptr);

}  // end namespace pmt

#endif
