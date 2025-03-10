#include <filesystem>

#include "common/Exception.h"
#include "Powermon.h"

Powermon::Powermon(const std::string& base_path, unsigned int id) {
  const std::string filename_label =
      base_path + "/in" + std::to_string(id) + "_label";
  const std::string filename_voltage =
      base_path + "/in" + std::to_string(id) + "_input";
  const std::string filename_current =
      base_path + "/curr" + std::to_string(id) + "_input";

  if (std::filesystem::exists(filename_label) &&
      std::filesystem::exists(filename_voltage) &&
      std::filesystem::exists(filename_current)) {
    std::ifstream(filename_label) >> label_;
    voltage_ = std::make_shared<std::ifstream>(filename_voltage);
    current_ = std::make_shared<std::ifstream>(filename_current);
  } else {
    std::stringstream message;
    message << "Failed to initialize powermon " << std::to_string(id) << " at "
            << base_path;
    throw pmt::Exception(message.str().c_str());
  }
}

float Powermon::read() {
  voltage_->clear();
  voltage_->seekg(0);
  std::string voltage_str;
  std::getline(*voltage_, voltage_str);
  const unsigned int voltage = std::atoi(voltage_str.c_str());  // mV

  current_->clear();
  current_->seekg(0);
  std::string current_str;
  std::getline(*current_, current_str);
  const unsigned int current = std::atoi(current_str.c_str());  // mA

  return voltage * current * 1.e-6;  // W
}