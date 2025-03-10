#ifndef POWERMON_H_
#define POWERMON_H_

#include <fstream>
#include <memory>
#include <string>

class Powermon {
 public:
  Powermon(const std::string& base_path, unsigned int id);
  Powermon(Powermon&& other) noexcept = default;
  std::string name() { return label_; }
  float read();  // W

 private:
  std::string label_;
  std::shared_ptr<std::ifstream> voltage_;
  std::shared_ptr<std::ifstream> current_;
};

#endif  // POWERMON_H_