#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

#include <ext/alloc_traits.h>
#include <signal.h>

#include "common/Exception.h"
#include "TegraImpl.h"

namespace {
bool FileExists(const std::string& name) {
  if (FILE* file = fopen(name.c_str(), "r")) {
    fclose(file);
    return true;
  } else {
    return false;
  }
}

std::string Execute(const std::string& commandline) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(commandline.c_str(), "r"),
                                                pclose);
  if (!pipe) {
    throw pmt::Exception("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}

std::vector<std::string> SplitString(const std::string& string,
                                     std::string delimiter = " ") {
  std::vector<std::string> substrings;
  int start = 0;
  int end = string.find_first_of(" \n");
  while (end != -1) {
    start = end + delimiter.size();
    end = string.find(delimiter, start);
    substrings.push_back(string.substr(start, end - start));
  }
  substrings.push_back(string.substr(start, end - start));
  return substrings;
}

std::string FindLogfile() {
  const std::string ps = Execute("ps aux | grep tegrastats");
  std::vector<std::string> substrings = SplitString(ps);
  bool have_tegrastats = false;
  int index_logfile = -1;
  for (size_t i = 0; i < substrings.size(); ++i) {
    if (substrings[i].compare("tegrastats") == 0 ||
        substrings[i].compare("/usr/bin/tegrastats") == 0) {
      have_tegrastats = true;
    }
    if (substrings[i].compare("--logfile") == 0) {
      index_logfile = i + 1;
      break;
    }
  }
  if (have_tegrastats && index_logfile > 0) {
    return substrings[index_logfile];
  } else {
    return "";
  }
}

std::string StartTegraStats(int interval) {
  char filename[32] = "/tmp/tegrastats-XXXXXX";
  if (mkstemp(filename) == -1) {
    throw pmt::Exception("Could not create temporary file");
  }
  const char* binary = "/usr/bin/tegrastats";
  std::ostringstream commandline;
  commandline << binary << " --start --interval " << interval << " --logfile "
              << filename;
  std::string output = Execute(commandline.str());
  if (output.compare("") != 0) {
    std::ostringstream message;
    message << "Error starting tegrastats: " << output;
    throw pmt::Exception(message.str().c_str());
  }
  return filename;
}

void StopTegraStats(const std::string& logfile) {
  if (logfile.compare("") != 0) {
    const char* binary = "/usr/bin/tegrastats";
    std::ostringstream commandline;
    commandline << binary << " --stop";
    Execute(commandline.str());
    std::remove(logfile.c_str());
  }
}

std::string ReadLastLine(const std::string& file_name) {
  std::ifstream file(file_name);
  if (!file.is_open()) {
    return "";
  }

  std::string last_line;
  std::string current_line;

  while (std::getline(file, current_line)) {
    if (!current_line.empty()) {
      last_line = current_line;
    }
  }

  file.close();
  return last_line;
}

std::vector<pmt::tegra::TegraMeasurement> ReadPowerMeasurements(
    const std::string& line) {
  std::vector<pmt::tegra::TegraMeasurement> measurements;

  std::vector<std::regex> regexes;
  regexes.emplace_back(
      "(\\w+) (\\d+)mW\\/(\\d+)mW");  // Jetson AGX Xavier and Jetson Orin Nano
  regexes.emplace_back("(POM_\\w+) (\\d+)\\/(\\d+)");  // Jetson Nano

  for (std::regex& regex : regexes) {
    std::smatch matches;
    std::string input = line;

    while (std::regex_search(input, matches, regex)) {
      const std::string name = matches[1].str();
      const int instantaneous_mw = atoi(matches[2].str().c_str());
      const int average_mw = atoi(matches[3].str().c_str());
      measurements.emplace_back(name, instantaneous_mw);
      input = matches.suffix();
    }
  }

  return measurements;
}

bool CheckSensors(
    const std::vector<std::string>& sensors,
    const std::vector<pmt::tegra::TegraMeasurement>& measurements) {
  if (sensors.size() != measurements.size()) {
    return false;
  }
  const size_t n = sensors.size();
  for (size_t i = 0; i < n; i++) {
    if (sensors[i].compare(measurements[i].first) != 0) {
      return false;
    }
  }
  return true;
}
}  // end namespace

namespace pmt::tegra {

void SignalCallbackHandler(int num) {
  const std::string logfile = ::FindLogfile();
  ::StopTegraStats(logfile);
  signal(SIGINT, SIG_DFL);
  raise(num);
}

TegraImpl::TegraImpl() {
  filename_ = ::FindLogfile();
  if (!::FileExists(filename_)) {
    ::StopTegraStats(filename_);
    filename_ = ::StartTegraStats(measurement_interval_);
    started_tegrastats_ = true;
    signal(SIGINT, SignalCallbackHandler);
  }
}

TegraImpl::~TegraImpl() {
  if (started_tegrastats_) {
    ::StopTegraStats(filename_);
    started_tegrastats_ = false;
  }
}

std::vector<TegraMeasurement> TegraImpl::GetMeasurements() {
  std::string line = ::ReadLastLine(filename_);

  while (line.compare("") == 0) {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(measurement_interval_));
    line = ::ReadLastLine(filename_);
  }

  return ::ReadPowerMeasurements(line);
};

const std::vector<std::string> sensors_agx_xavier{"GPU", "CPU",   "SOC",
                                                  "CV",  "VDDRQ", "SYS5V"};
const std::vector<std::string> sensors_agx_orin{
    "VDD_GPU_SOC", "VDD_CPU_CV", "VIN_SYS_5V0", "VDDQ_VDD2_1V8AO"};
const std::vector<std::string> sensors_jetson_nano{"POM_5V_IN", "POM_5V_GPU",
                                                   "POM_5V_CPU"};
const std::vector<std::string> sensors_jetson_orin_nano{
    "VDD_IN", "VDD_CPU_GPU_CV", "VDD_SOC"};

State TegraImpl::GetState() {
  const std::vector<TegraMeasurement> measurements = GetMeasurements();
  State state(1 + measurements.size());
  state.timestamp_ = GetTime();
  state.name_[0] = "total";

  // Compute total power consumption as sum of individual measurements.
  // Which individual measurements to use differs per platform.
  state.watt_[0] = 0;
  if (::CheckSensors(sensors_agx_xavier, measurements)) {
    // Jetson AGX Xavier: sum all sensors
    for (auto& measurement : measurements) {
      state.watt_[0] += measurement.second;
    }
  } else if (::CheckSensors(sensors_agx_orin, measurements)) {
    // Jetson AGX Orin: sum all sensors
    for (auto& measurement : measurements) {
      state.watt_[0] += measurement.second;
    }
  } else if (::CheckSensors(sensors_jetson_nano, measurements)) {
    // Jetson Nano: POM_5V_IN only
    state.watt_[0] += measurements[0].second;
  } else if (::CheckSensors(sensors_jetson_orin_nano, measurements)) {
    // Jetson Nano: VDD_IN only
    state.watt_[0] += measurements[0].second;
  } else {
    throw pmt::Exception("Unknown Jetson device.");
  }
  for (size_t i = 0; i < measurements.size(); i++) {
    const std::string name = measurements[i].first;
    const double watt = measurements[i].second / 1.e3;
    state.name_[i + 1] = name;
    state.watt_[i + 1] = watt;
  }
  state.watt_[0] /= 1e3;

  return state;
}

}  // end namespace pmt::tegra