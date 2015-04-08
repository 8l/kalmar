// RUN: %cxxamp %s -o %t.out && %t.out
#include <amp.h>
#include <stdlib.h>
#include <iostream>
#include <thread>
using namespace concurrency;

void thread_body(int tid) {
  std::vector<accelerator> accs = accelerator::get_all();
  accelerator device = accs[tid];
  if (device == accelerator(Concurrency::accelerator::cpu_accelerator))
    return;
  if (device.get_device_path() != (L"gpu" + std::to_wstring(tid-1))) {
    std::cout << "path error!\n";
    exit(1);
  }

  accelerator_view rv = device.get_default_view();
  if (rv.get_accelerator() != device) {
    std::cout << "get_default_view error!\n";
    exit(1);
  }

  const int vecSize = 100;

  // Alloc & init input data on rv
  Concurrency::extent<1> e(vecSize);
  Concurrency::array<int, 1> a(vecSize, rv);
  Concurrency::array<int, 1> b(vecSize, rv);
  Concurrency::array<int, 1> c(vecSize, rv);

  Concurrency::array_view<int> ga(a); 
  Concurrency::array_view<int> gb(b);
  Concurrency::array_view<int> gc(c);
  accelerator_view gav = ga.get_source_accelerator_view();
  if (gav != rv) {
    std::cout << "get_source_accelerator_view error!\n";
    exit(1);
  }

  if (gav.get_accelerator() != device) {
    std::cout << "get_accelerator() error!\n";
    exit(1);
  }
}

#define NUM 64
int main () {
  std::vector<accelerator> accs = accelerator::get_all();
  std::thread th[NUM];
  for (int i = 0; i < accs.size(); i++) {
    th[i] = std::thread(thread_body, i);
  }

  for (int i = 0; i < accs.size(); i++) {
   th[i].join();
  }

  return 0;
}

