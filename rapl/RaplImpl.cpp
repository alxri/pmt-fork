#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <memory>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

#include "RaplImpl.h"
#include "RaplCounter.h"

#include "common/cpu.h"

/*
 * RAPL (Running Average Power Limit) is an API provided by Intel for power
 * monitoring on Intel hardware. It allows access to energy consumption data for
 * each socket and its components.
 *
 * The API provides:
 * - Total energy consumption per socket:
 *   /sys/class/powercap/intel-rapl:X/energy_uj
 *
 * For each socket, it also breaks down the energy consumption into:
 * - Core energy:
 *   /sys/class/powercap/intel-rapl:X:0
 * - Non-core energy:
 *   /sys/class/powercap/intel-rapl:X:1
 */

namespace pmt::rapl {

RaplImpl::RaplImpl() {
  Init();
  previous_timestamp_ = GetTime();
  previous_measurements_ = GetMeasurements();
}

RaplImpl::~RaplImpl() { std::lock_guard<std::mutex> lock(mutex_); }

std::string GetBasename(int package_id) {
  std::stringstream basename;
  basename << "/sys/class/powercap/intel-rapl:" << package_id;
  return basename.str();
}

void RaplImpl::Init() {
  const fs::path basename = "/sys/class/powercap";
  const std::regex pattern_directory("intel-rapl(:\\d+)+");
  const std::regex pattern_package_id(":([0-9]+)");

  std::vector<std::string> rapl_dirs;

  const std::set active_sockets =
      common::get_active_packages(common::get_active_cpus());

  for (const auto& entry : fs::directory_iterator(basename)) {
    const std::string directory = entry.path().filename().string();

    std::smatch match;
    if (std::regex_match(directory, pattern_directory)) {
      if (fs::exists(entry.path() / "energy_uj")) {
        if (std::regex_search(directory, match, pattern_package_id)) {
          const int package_id = std::stoi(match[1].str());
          if (active_sockets.find(package_id) != active_sockets.end()) {
            rapl_dirs.emplace_back(entry.path());
          }
        }
      }
    }
  }

  if (rapl_dirs.empty()) {
    std::cerr
        << "Warning: no RAPL interface detected. Please make sure that the "
           "system supports RAPL and that you have the necessary permissions."
        << std::endl;
  }

  for (const auto& rapl_dir : rapl_dirs) {
    try {
      rapl_counters_.emplace_back(rapl_dir);
    } catch (std::system_error& e) {
      std::stringstream message;
      message << "OS error: " << e.what();
      std::cerr << message.str() << std::endl;
      if (e.code().value() == EACCES) {
        std::cerr << "Please check the permissions or try to run as 'root'"
                  << std::endl;
      }
    }
  }
}

std::vector<RaplMeasurement> RaplImpl::GetMeasurements() {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<RaplMeasurement> measurements;

  for (auto& counter : rapl_counters_) {
    measurements.emplace_back(counter.GetName(), counter.Read());
  }

  return measurements;
}

State RaplImpl::GetState() {
  std::vector<RaplMeasurement> measurements = GetMeasurements();
  State state(1 + measurements.size());
  state.timestamp_ = GetTime();
  state.name_[0] = "total";
  state.joules_[0] = 0;
  state.watt_[0] = 0;

  for (std::size_t i = 0; i < measurements.size(); i++) {
    const std::string name = measurements[i].name;
    const std::size_t ujoules_now = measurements[i].ujoules;
    const std::size_t ujoules_previous = previous_measurements_[i].ujoules;
    const double duration = seconds(previous_timestamp_, state.timestamp_);
    const float joules_diff = (ujoules_now - ujoules_previous) * 1e-6;
    const float watt = joules_diff / duration;
    state.name_[i + 1] = name;
    state.joules_[i + 1] = ujoules_now * 1e-6;
    state.watt_[i + 1] = watt;

    if (name.find("package") != std::string::npos) {
      state.joules_[0] += ujoules_now * 1e-6;
      state.watt_[0] += watt;
    }
  }

  previous_timestamp_ = state.timestamp_;
  previous_measurements_ = measurements;

  return state;
}

}  // namespace pmt::rapl
