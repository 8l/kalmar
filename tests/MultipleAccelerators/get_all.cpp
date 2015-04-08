// RUN: %cxxamp %s -o %t.out && %t.out
#include <amp.h>
#include <stdlib.h>
#include <iostream>
#include <thread>
using namespace concurrency;

int main() {
  std::vector<accelerator> accs = accelerator::get_all();
  int num = accs.size();
  if (!num)
    return 1;
  std::vector<accelerator> accs_again = accelerator::get_all();
  if (! accs_again.size())
    return 1;
  bool has_cpu_acc = false;
  std::map<cl_device_id, cl_device_id> ids;
  for (auto &acc : accs) {
    if (acc.get_device_path() == accelerator::cpu_accelerator)
      has_cpu_acc = true;
    ids[acc.get_device_id()] = acc.get_device_id();
  }
  if (!has_cpu_acc)
    return 1;

  if (ids.size() != num)
    return 1;

  return 0;
}
