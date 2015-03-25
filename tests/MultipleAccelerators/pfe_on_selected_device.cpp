// RUN: %cxxamp %s -o %t.out && %t.out
#include <amp.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
using namespace concurrency;

int main() {
  std::vector<accelerator> accs = accelerator::get_all();
  std::wstring specified_device;
  
  if (accs.size() < 3 ) {
    // 1GPU
    specified_device = Concurrency::accelerator::default_accelerator;
  } else  {
    // 2GPUs
    specified_device = L"gpu1"; // whatever from L"gpu0" to L"gpuN"
  }
   
  accelerator device = accelerator(specified_device);
  accelerator_view rv = device.get_default_view();
  const int vecSize = 100;

  // Alloc & init input data
  Concurrency::extent<1> e(vecSize);
  Concurrency::array<int, 1> a(vecSize);
  Concurrency::array<int, 1> b(vecSize);
  Concurrency::array<int, 1> c(vecSize);
  int sum = 0;
  
  for (Concurrency::index<1> i(0); i[0] < vecSize; i++) {
    a[i] = 100.0f * rand() / RAND_MAX;
    b[i] = 100.0f * rand() / RAND_MAX;
    sum += a[i] + b[i];
  }

  Concurrency::array_view<int> ga(a);
  Concurrency::array_view<int> gb(b);
  Concurrency::array_view<int> gc(c);
  Concurrency::parallel_for_each(rv,
    e,
    [=](Concurrency::index<1> idx) restrict(amp) {
    gc[idx] = ga[idx]+gb[idx];
  });

  int error = 0;
  for(unsigned i = 0; i < vecSize; i++) {
    error += gc[i] - (ga[i] + gb[i]);
  }
  return (error != 0);
}
