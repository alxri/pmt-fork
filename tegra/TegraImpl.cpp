#include <filesystem>
#include <iostream>
#include <regex>
#include <sstream>
#include <vector>

#include "Powermon.h"
#include "TegraImpl.h"

namespace {
std::vector<Powermon> initialize_powermons(const std::string &base_path) {
  std::vector<Powermon> powermons;
  std::vector<std::pair<std::string, std::shared_ptr<std::ifstream>>>
      hwmon_inputs;
  std::regex label_regex("in(\\d+)_label");
  std::regex valid_label_regex("^[A-Z0-9]+(_[A-Z0-9]+)*$");

  for (const auto &hwmon_dir : std::filesystem::directory_iterator(base_path)) {
    if (!std::filesystem::is_directory(hwmon_dir)) continue;

    for (const auto &entry : std::filesystem::directory_iterator(hwmon_dir)) {
      std::string filename = entry.path().filename().string();
      std::smatch match;
      if (std::regex_match(filename, match, label_regex)) {
        std::ifstream label_file(entry.path());
        if (label_file) {
          std::string label_content;
          label_file >> label_content;

          // Skip labels that don't match the required pattern
          if (std::regex_match(label_content, valid_label_regex)) {
            powermons.emplace_back(hwmon_dir.path().string(),
                                   std::stoi(match[1].str()));
          }
        }
      }
    }
  }

  return powermons;
}

}  // end namespace

namespace pmt::tegra {

TegraImpl::TegraImpl() {
  powermons_ = ::initialize_powermons("/sys/class/hwmon");
}

State TegraImpl::GetState() {
  State state(1 + powermons_.size());
  state.timestamp_ = GetTime();
  state.name_[0] = "total";
  state.watt_[0] = 0.f;

  for (int i = 0; i < powermons_.size(); i++) {
    state.name_[i + 1] = powermons_[i].name();
    state.watt_[i + 1] = powermons_[i].read();
    state.watt_[0] += state.watt_[i];
  }

  return state;
}

}  // end namespace pmt::tegra