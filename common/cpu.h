#ifndef PMT_CPU_H_
#define PMT_CPU_H_

#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <unistd.h>

#include "common/Exception.h"

namespace pmt::common {

std::vector<int> get_active_cpus() {
  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);

  const int result = sched_getaffinity(0, sizeof(cpu_set), &cpu_set);
  if (result == -1) {
    throw pmt::Exception("sched_getaffinity");
  }

  const int n_cpus = sysconf(_SC_NPROCESSORS_ONLN);

  std::vector<int> active_cpus;
  for (int cpu = 0; cpu < n_cpus; ++cpu) {
    if (CPU_ISSET(cpu, &cpu_set)) {
      active_cpus.push_back(cpu);
    }
  }

  return active_cpus;
}

std::set<int> get_active_packages(const std::vector<int>& active_cpus) {
  std::set<int> active_packages;

  for (int cpu : active_cpus) {
    const std::string path = "/sys/devices/system/cpu/cpu" +
                             std::to_string(cpu) +
                             "/topology/physical_package_id";

    std::ifstream file(path);
    if (!file.is_open()) {
      std::ostringstream message;
      message << "Failed to open file: " << path;
      throw pmt::Exception(message.str().c_str());
    }

    int socket_id;
    file >> socket_id;

    active_packages.insert(socket_id);
  }

  return active_packages;
}

}  // namespace pmt::common

#endif  // PMT_CPU_H_