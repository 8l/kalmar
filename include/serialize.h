//===----------------------------------------------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#pragma once

namespace Concurrency {
#if defined(CXXAMP_ENABLE_HSA)
namespace CLAMP {
extern void HSAPushArg(void *, size_t, const void *);
extern void HSAPushPointer(void *, void *);
}
#endif
#ifdef __CPU_PATH__
class Serialize {
public:
  void Append(size_t sz, const void *s) {}
  void AppendPtr(const void *ptr) {}
};
#else
class Serialize {
 public:
#if defined(CXXAMP_ENABLE_HSA)
  typedef void *hsa_kernel;
  Serialize(hsa_kernel k): k_(k) {}
#else
  Serialize(ecl_kernel k): k_(k), current_idx_(0) {}
#endif
  void Append(size_t sz, const void *s) {
#if defined(CXXAMP_ENABLE_HSA)
    CLAMP::HSAPushArg(k_, sz, s);
#else
    ecl_error err;
    err = eclSetKernelArg(k_, current_idx_++, sz, s);
    assert(err == eclSuccess);
#endif
  }
  void AppendPtr(const void *ptr) {
#if defined(CXXAMP_ENABLE_HSA)
    CLAMP::HSAPushPointer(k_, const_cast<void*>(ptr));
#else
    ecl_error err;
    err = eclSetKernelArgPtr(k_, current_idx_++, ptr);
    assert(err == eclSuccess);
#endif
  }
 private:
#if defined(CXXAMP_ENABLE_HSA)
  hsa_kernel k_;
#else
  ecl_kernel k_;
#endif
  int current_idx_;
};
#endif
}
