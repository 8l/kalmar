// RUN: %cxxamp %s -o %t.out && %t.out
#include <amp.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
using namespace concurrency;

int main() {
  std::vector<accelerator> accs = accelerator::get_all();
  std::wstring src_device;
  std::wstring dest_device;
  
  if (accs.size() < 3 ) {
    // 1GPU, device2device copy
    src_device = Concurrency::accelerator::default_accelerator;
    dest_device = Concurrency::accelerator::default_accelerator;
  } else  {
    // 2GPUs, src/dest device is different. peer2peer copy
    src_device = L"gpu0";
    dest_device = L"gpu1"; // whatever from L"gpu0" to L"gpuN"
  }
   
  const int vecSize = 100;
  // Alloc & init input data on CPU
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
  // Combine array_views with arrays
  Concurrency::array_view<int> ga(a);
  Concurrency::array_view<int> gb(b);
  Concurrency::array_view<int> gc(c);

  // Specify to compute on source device
  accelerator src_acc = accelerator(src_device);
  accelerator_view src_accl_view = src_acc.get_default_view();
  // After pfe, array_views hold data on source device
  Concurrency::parallel_for_each(src_accl_view,
    e,
    [=](Concurrency::index<1> idx) restrict(amp) {
    gc[idx] = ga[idx]+gb[idx];
  });

  // Do the same computation on destination device
  accelerator dest_acc = accelerator(dest_device);
  accelerator_view dest_accl_view = dest_acc.get_default_view();
  /////////////////////////////////////////////////////
  // Before the followed pfe, data source of array_views shall be
  // created on destination device if it is not the same as
  // source device. The copying of its containing data from 
  // source device to destination device shall happen 
  // transparently as well at AMP runtime.
  //////////////////////////////////////////////////////
  Concurrency::parallel_for_each(dest_accl_view,
    e,
    [=](Concurrency::index<1> idx) restrict(amp) {
    gc[idx] = gc[idx] + ga[idx]+gb[idx];
  });

  int error = 0;
  for(unsigned i = 0; i < vecSize; i++) {
    error += gc[i] - 2*(ga[i] + gb[i]);
  }
  return (error != 0);
}
