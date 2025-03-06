#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>

#include <pmt.h>

#include "common/Exception.h"
#include "common/PMT.h"

namespace {
template <typename T>
bool isEqual(T x, T y) {
  return std::fabs(x - y) <= std::numeric_limits<T>::epsilon();
}

bool isNumber(const std::string &s) {
  return !s.empty() && std::all_of(s.begin(), s.end(),
                                   [](char c) { return std::isdigit(c); });
}
}  // end namespace

namespace pmt {

PMT::~PMT() {
  StopDump();
  StopThread();
};

double PMT::seconds(const Timestamp &timestamp) {
  return std::chrono::duration_cast<std::chrono::microseconds>(
             timestamp.time_since_epoch())
             .count() /
         1.e6;
}

double PMT::seconds(const Timestamp &first, const Timestamp &second) {
  return std::chrono::duration_cast<std::chrono::microseconds>(second - first)
             .count() /
         1e6;
}

double PMT::seconds(const State &first, const State &second) {
  return seconds(first.timestamp_, second.timestamp_);
}

double PMT::joules(const State &first, const State &second) {
  return second.joules_[0] - first.joules_[0];
}

double PMT::watts(const State &first, const State &second) {
  return joules(first, second) / seconds(first, second);
}

unsigned int PMT::GetDumpInterval() {
  const char *dump_interval_ = std::getenv(kDumpIntervalVariable.c_str());
  if (dump_interval_) {
    unsigned int dump_interval = std::stoi(dump_interval_);
    assert(dump_interval > 0);
    return dump_interval;
  } else {
    return GetMeasurementInterval();
  }
}

void PMT::StartThread() {
  thread_mutex_.lock();

  thread_ = std::thread([&] {
    try {
      state_latest_ = GetState();
    } catch (const pmt::Exception &e) {
#if defined(DEBUG)
      std::cerr << "GetState(): " << e.what() << std::endl;
#endif
      throw pmt::Exception("Could not read initial state.");
    }

    if (dump_file_) {
      DumpHeader(state_latest_);
    }

    while (!thread_stop_) {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(measurement_interval_));

      State state;
      try {
        state = GetState();
      } catch (const pmt::Exception &e) {
#if defined(DEBUG)
        std::cerr << "GetState(): " << e.what() << std::endl;
#endif
        continue;
      }

      if (dump_file_ &&
          (1e3 * seconds(state_latest_, state)) > GetDumpInterval()) {
        Dump(state);
      }

      for (int i = 0; i < state.NrMeasurements(); i++) {
        const double seconds_diff = seconds(state_latest_, state);
        if (state.watt_[i] > 0) {
          const double watt_average =
              (state_latest_.watt_[i] + state.watt_[i]) / 2.f;
          state.joules_[i] =
              state_latest_.joules_[i] + watt_average * seconds_diff;
        } else if (state.joules_[i] > 0) {
          const double joules_diff =
              state.joules_[i] - state_latest_.joules_[i];
          state.watt_[i] = joules_diff / seconds_diff;
        }
      }

      state_latest_ = state;

      if (!thread_started_) {
        thread_mutex_.unlock();
        thread_started_ = true;
      }
    }
  });
}

void PMT::StopThread() {
  thread_stop_ = true;
  if (thread_.joinable()) {
    thread_.join();
  }
}

void PMT::StartDump(const char *filename) {
  const char *filename_ = std::getenv(kDumpFilenameVariable.c_str());
  if (filename_) {
    filename = filename_;
  }
  if (!filename) {
    filename = GetDumpFilename();
  }
  assert(filename);

  dump_file_ = std::make_unique<std::ofstream>(filename);
  Read();
}

void PMT::StopDump() { dump_file_.reset(); }

void PMT::DumpHeader(const State &state) {
  if (dump_file_ != nullptr) {
    std::unique_lock<std::mutex> lock(dump_file_mutex_);
    *dump_file_ << "timestamp";
    for (const std::string &name : state.name_) {
      *dump_file_ << " " << name;
    }
    *dump_file_ << std::endl;
  }
}

void PMT::Dump(const State &state) {
  if (dump_file_ != nullptr) {
    std::unique_lock<std::mutex> lock(dump_file_mutex_);
    *dump_file_ << std::fixed << std::setprecision(3)
                << seconds(state.timestamp_);
    for (float watt : state.watt_) {
      *dump_file_ << " " << watt;
    }
    *dump_file_ << std::endl;
  }
}

void PMT::Mark(const State &state, const std::string &message) const {
  if (dump_file_ != nullptr) {
    std::unique_lock<std::mutex> lock(dump_file_mutex_);
    *dump_file_ << "M " << std::fixed << seconds(state.timestamp_) << " \""
                << message << "\"" << std::endl;
  }
}

Timestamp PMT::GetTime() { return std::chrono::system_clock::now(); }

State PMT::Read() {
  const int measurement_interval = GetMeasurementInterval();
  if (!thread_started_) {
    StartThread();

    unsigned int retries = 0;

    do {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(measurement_interval * (retries + 1)));
      if (retries++ > kNrRetriesInitialization) {
        throw pmt::Exception("Measurement thread failed to start");
      }
    } while (!thread_mutex_.try_lock());

    state_returned_ = state_latest_;
    return state_latest_;
  }

  State state = state_latest_;
  if (seconds(state_returned_, state) <= 0) {
    state = state_returned_;
    state.timestamp_ = GetTime();
    const double duration = seconds(state_returned_, state);
    for (int i = 0; i < state.NrMeasurements(); i++) {
      state.joules_[i] += state.watt_[i] * duration;
    }
  }

  return state_returned_ = state;
}

std::unique_ptr<PMT> Create(const std::string &name,
                            const std::string &argument) {
  if (argument.empty()) {
    // Create PMT instance without argument
#if defined(PMT_BUILD_CRAY)
    if (name == cray::Cray::name) {
      return cray::Cray::Create();
    }
#endif
#if defined(PMT_BUILD_LIKWID)
    if (name == likwid::Likwid::name) {
      return likwid::Likwid::Create();
    }
#endif
#if defined(PMT_BUILD_RAPL)
    if (name == rapl::Rapl::name) {
      return rapl::Rapl::Create();
    }
#endif
#if defined(PMT_BUILD_TEGRA)
    if (name == tegra::Tegra::name) {
      return tegra::Tegra::Create();
    }
#endif
#if defined(PMT_BUILD_NVML)
    if (name == nvml::NVML::name) {
      return nvml::NVML::Create();
    }
#endif
#if defined(PMT_BUILD_NVIDIA)
    if (name == nvidia::NVIDIA::name) {
      return nvidia::NVIDIA::Create();
    }
#endif
#if defined(PMT_BUILD_AMDSMI)
    if (name == amdsmi::AMDSMI::name) {
      return amdsmi::AMDSMI::Create();
    }
#endif
#if defined(PMT_BUILD_ROCM)
    if (name == rocm::ROCM::name) {
      return rocm::ROCM::Create();
    }
#endif
  } else if (isNumber(argument)) {
    // Create PMT instance with device number
    const int device_number = std::stoi(argument);

#if defined(PMT_BUILD_NVML)
    if (name == nvml::NVML::name) {
      return nvml::NVML::Create(device_number);
    }
#endif
#if defined(PMT_BUILD_NVIDIA)
    if (name == nvidia::NVIDIA::name) {
      return nvidia::NVIDIA::Create(device_number);
    }
#endif
#if defined(PMT_BUILD_AMDSMI)
    if (name == amdsmi::AMDSMI::name) {
      return amdsmi::AMDSMI::Create(device_number);
    }
#endif
#if defined(PMT_BUILD_ROCM)
    if (name == rocm::ROCM::name) {
      return rocm::ROCM::Create(device_number);
    }
#endif
  } else {
    // Create PMT instance with string argument

    if (argument.size() > 1) {
#if defined(PMT_BUILD_POWERSENSOR2)
      if (name == powersensor2::PowerSensor2::name) {
        return powersensor2::PowerSensor2::Create(argument.c_str());
      }
#endif
#if defined(PMT_BUILD_POWERSENSOR3)
      if (name == powersensor3::PowerSensor3::name) {
        return powersensor3::PowerSensor3::Create(argument.c_str());
      }
#endif
#if defined(PMT_BUILD_XILINX)
      if (name == xilinx::Xilinx::name) {
        return xilinx::Xilinx::Create(argument.c_str());
      }
#endif
    }
  }

#if defined(DEBUG)
  std::ostringstream message;
  message << "Invalid or unavailable platform specified: " << name << std::endl;
  throw pmt::Exception(message.str().c_str());
#endif

  return Dummy::Create();
}
}  // end namespace pmt

std::ostream &operator<<(std::ostream &os, const pmt::Timestamp &timestamp) {
  os << pmt::PMT::seconds(timestamp);
  return os;
}
