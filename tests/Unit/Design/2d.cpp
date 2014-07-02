// RUN: %amp_device -D__GPU__ %s -m32 -emit-llvm -c -S -O2 -o %t.ll 
// RUN: mkdir -p %t
// RUN: %clamp-device %t.ll %t/kernel.cl 
// RUN: pushd %t 
// RUN: %embed_kernel kernel.cl kernel.o
// RUN: popd
// RUN: %gtest_amp %t/kernel.o %s -o %t.out 
// RUN: %t.out

#include <amp.h>
#include <stdlib.h>
#include <iostream>
#ifndef __GPU__
#include <gtest/gtest.h>
#endif

class myVecAdd {
 public:
  // CPU-side constructor. Written by the user
  myVecAdd(Concurrency::array_view<int, 2>& a,
    Concurrency::array_view<int, 2> &b,
    Concurrency::array<int, 2> &c):
    a_(a), b_(b), c_(c) {
  }
  void operator() (Concurrency::index<2> idx) const restrict(amp) {
    c_[idx] = a_[idx]+b_[idx];
  }
  void operator() (Concurrency::tiled_index<4, 4> idx) const restrict(amp) {
    c_[idx] = a_[idx]+b_[idx];
  }
 private:
  Concurrency::array<int, 2> &c_;
  Concurrency::array_view<int, 2> a_, b_;
};
void bar(void) restrict(amp,cpu) {
  int* foo = reinterpret_cast<int*>(&myVecAdd::__cxxamp_trampoline);
}
#ifndef __GPU__
#define M 20
#define N 40
TEST(Design, Final) {
  std::vector<int> vector_a(M*N),
                   vector_b(M*N);
  for (int i = 0; i < M*N; i++) {
    vector_a[i] = 100.0f * rand() / RAND_MAX;
    vector_b[i] = 100.0f * rand() / RAND_MAX;
  }
  Concurrency::extent<2> e(M, N);
  concurrency::array_view<int, 2> av(e, vector_a);
  EXPECT_EQ(vector_a[2], av(0,2));
  concurrency::array_view<int, 2> bv(e, vector_b);
  { // Test untiled version
    concurrency::array<int, 2> c(e);
    myVecAdd mf(av, bv, c);
    Concurrency::parallel_for_each(e, mf);
    int error=0;
    for(int i = 0; i < M; i++) {
      for(int j = 0; j < N; j++) {
	std::cout << "av[" <<i<<","<<j<<"] = "<<av(i,j)<<"\n";
	std::cout << "bv[" <<i<<","<<j<<"] = "<<bv(i,j)<<"\n";
	std::cout << "c[" <<i<<","<<j<<"] = "<<c(i,j)<<"\n";
	error += abs(c(i, j) - (av(i, j) + bv(i, j)));
      }
    }
    EXPECT_EQ(0, error);
  }
  {
   // Test tiled version
    concurrency::array<int, 2> c(e);
    myVecAdd mf(av, bv, c);
    Concurrency::parallel_for_each(e.tile<4, 4>(), mf);
    int error=0;
    for(int i = 0; i < M; i++) {
      for(int j = 0; j < N; j++) {
	std::cout << "av[" <<i<<","<<j<<"] = "<<av(i,j)<<"\n";
	std::cout << "bv[" <<i<<","<<j<<"] = "<<bv(i,j)<<"\n";
	std::cout << "c[" <<i<<","<<j<<"] = "<<c(i,j)<<"\n";
	error += abs(c(i, j) - (av(i, j) + bv(i, j)));
      }
    }
    EXPECT_EQ(0, error);
  }
}
#endif
