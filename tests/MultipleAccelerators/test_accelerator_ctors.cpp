// RUN: %cxxamp %s -o %t.out && %t.out
#include <amp.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
using namespace concurrency;

int main() {
  std::vector<accelerator> accs = accelerator::get_all();
  int num = accs.size();
  // Only test > 2GPUs
  if (accs.size() < 2 )
    return 0;

  std::vector<cl_device_id> ids;
  cl_device_id cpu_did = NULL;
  cl_device_id default_acc_did = NULL;
  cl_device_id gpu0_did = NULL;
  
  for (auto acc: accs) {
    ids.push_back(acc.get_device_id());
    if (acc.get_device_path() == Concurrency::accelerator::cpu_accelerator)
      cpu_did = acc.get_device_id();
    // FIXME: not matched for default_accelerator, it is L"gpu0". Fix later
    /*else if (acc.get_device_path() == Concurrency::accelerator::default_accelerator)
      default_acc_did = acc.get_device_id();
    std::wcout << "path = " << acc.get_device_path() << L"\n";*/
  }
 
  if (!cpu_did) {
    std::cout << "no cpu accelerator constructed!\n";
    exit(1);
  }

  /*if (!default_acc_did) {
    std::cout << "no default accelerator constructed!\n";
    exit(1);
  }*/

  // Construt default accelerator
 {
  accelerator device = accelerator(Concurrency::accelerator::default_accelerator);
  auto iter = std::find(ids.begin(), ids.end(), device.get_device_id());
  if (iter == ids.end()) {
    std::cout << "constructing default accelerator error!\n";
    return 1;
  }
 } 
  // Construt cpu accelerator
 {
  accelerator device = accelerator(Concurrency::accelerator::cpu_accelerator);
  auto iter = std::find(ids.begin(), ids.end(), device.get_device_id());
  if (iter == ids.end()) {
    std::cout << "constructing cpu accelerator error!\n";
    return 1;
  }
 }
  
  // Construct gpu0-gpuN
 {
  for (int i = 0; i < num; i++) {
    accelerator device = accelerator(L"gpu" + std::to_wstring(i)) ;
    auto iter = std::find(ids.begin(), ids.end(), device.get_device_id());
    if (iter == ids.end()) {
      std::cout << "constructing default accelerator error!\n";
      return 1;
    }
  }

 }

  return 0;
}

