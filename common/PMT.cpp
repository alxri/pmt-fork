#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iomanip>

#include <pmt.h>

#include "common/PMT.h"
namespace pmt {

PMT::~PMT() {
  StopDump();
  StopThread();
};

double PMT::seconds(const State &first, const State &second) {
  return second.timestamp_ - first.timestamp_;
}

double PMT::joules(const State &first, const State &second) {
  return second.joules_[0] - first.joules_[0];
}

double PMT::watts(const State &first, const State &second) {
  return joules(first, second) / seconds(first, second);
}

float PMT::GetDumpInterval() {
  const char *dump_interval_ = std::getenv(kDumpIntervalVariable.c_str());
  return dump_interval_ ? std::stoi(dump_interval_)
                        : static_cast<float>(GetMeasurementInterval()) * 1e-3;
}

std::string State::name(int i) {
  assert(i < nr_measurements_);
  return name_[i];
}

float State::joules(int i) {
  assert(i < nr_measurements_);
  return joules_[i];
}

float State::watts(int i) {
  assert(i < nr_measurements_);
  return watt_[i];
}

void PMT::StartThread() {
  thread_ = std::thread([&] {
    const State start = GetState();
    assert(start.nr_measurements_ > 0);
    State previous = start;
    state_latest_ = start;

    if (dump_file_) {
      DumpHeader(start);
    }

    const int measurement_interval =
        GetMeasurementInterval();  // in milliseconds
    assert(measurement_interval > 0);
    const float dumpInterval = GetDumpInterval();  // in seconds
    assert(dumpInterval > 0);

    while (!thread_stop_) {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(measurement_interval));
      state_latest_ = GetState();

      const float duration = seconds(previous, state_latest_);
      if (dump_file_ && duration > dumpInterval) {
        Dump(start, previous, state_latest_);
        previous = state_latest_;
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

void PMT::Dump(const State &start, const State &first, const State &second) {
  if (dump_file_ != nullptr) {
    std::unique_lock<std::mutex> lock(dump_file_mutex_);
    *dump_file_ << std::fixed << std::setprecision(3) << seconds(start, second);
    for (float watt : second.watt_) {
      *dump_file_ << " " << watt;
    }
    *dump_file_ << std::endl;
  }
}

void PMT::Mark(const State &start, const State &current, const char *name,
               unsigned tag) const {
  if (dump_file_ != nullptr) {
    std::unique_lock<std::mutex> lock(dump_file_mutex_);
    *dump_file_ << "M " << current.timestamp_ - start.timestamp_ << ' ' << tag
                << " \"" << (name == nullptr ? "" : name) << '"' << std::endl;
  }
}

double PMT::GetTime() {
  return std::chrono::duration_cast<std::chrono::microseconds>(
             std::chrono::system_clock::now().time_since_epoch())
             .count() /
         1.0e6;
}

State PMT::Read() {
  const int measurement_interval = GetMeasurementInterval();
  if (!thread_started_) {
    StartThread();
    thread_started_ = true;
    std::this_thread::sleep_for(
        std::chrono::milliseconds(measurement_interval));
  }
  while (seconds(state_previous_, state_latest_) == 0) {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(measurement_interval));
  }
  state_previous_ = state_latest_;
  return state_latest_;
}

inline void create_warning(const std::string &name) {
#if defined(DEBUG)
  std::stringstream error;
  error << "Invalid or unavailable platform specified: " << name << std::endl;
  throw std::runtime_error(error.str());
#endif
}

std::unique_ptr<PMT> Create(const std::string &name, int argument) {
#if defined(PMT_BUILD_NVML)
  if (name == nvml::NVML::name) {
    return nvml::NVML::Create(argument);
  }
#endif
#if defined(PMT_BUILD_NVIDIA)
  if (name == nvidia::NVIDIA::name) {
    return nvidia::NVIDIA::Create(argument);
  }
#endif
#if defined(PMT_BUILD_AMDSMI)
  if (name == amdsmi::AMDSMI::name) {
    return amdsmi::AMDSMI::Create(argument);
  }
#endif
#if defined(PMT_BUILD_ROCM)
  if (name == rocm::ROCM::name) {
    return rocm::ROCM::Create(argument);
  }
#endif

  create_warning(name);
  return Dummy::Create();
}

std::unique_ptr<PMT> Create(const std::string &name, const char *argument) {
  int device_number = 0;

  if (argument == nullptr) {
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

  } else {
    if (std::strlen(argument) > 1) {
      // Create PMT instance with const char* argument
#if defined(PMT_BUILD_POWERSENSOR2)
      if (name == powersensor2::PowerSensor2::name) {
        return powersensor2::PowerSensor2::Create(argument);
      }
#endif
#if defined(PMT_BUILD_POWERSENSOR3)
      if (name == powersensor3::PowerSensor3::name) {
        return powersensor3::PowerSensor3::Create(argument);
      }
#endif
#if defined(PMT_BUILD_XILINX)
      if (name == xilinx::Xilinx::name) {
        return xilinx::Xilinx::Create(argument);
      }
#endif
    } else {
      // Overwrite the default device number
      device_number = std::atoi(argument);
    }
  }

  // Create PMT instance with integer argument
  return Create(name, device_number);

  create_warning(name);
  return Dummy::Create();
}

}  // end namespace pmt
