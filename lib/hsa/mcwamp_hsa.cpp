//===----------------------------------------------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// C++AMP Runtime implementation (HSA version)
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <map>
#include <vector>

#include <amp_allocator.h>
#include <amp_runtime.h>

#include "HSAContext.h"

extern "C" void PushArgImpl(void *ker, int idx, size_t sz, const void *v);
extern "C" void PushArgPtrImpl(void *ker, int idx, size_t sz, const void *v);

namespace Concurrency {
namespace CLAMP {

static HSAContext *context = NULL;

void FinalizeHSAContext() {
  if (context != NULL) {
    context->dispose();
    context = NULL;
  }

  // TBD dispose all Kernel objects

  // TBD dispose all Dispatch objects
}

/* Used only in HSA runtime */
HSAContext *GetOrInitHSAContext(void)
{
  if (!context) {
    //std::cerr << "CLAMP::HSA: create context\n";
    context = HSAContext::Create();
    atexit(FinalizeHSAContext); // register finalizer
  }
  if (!context) {
    std::cerr << "CLAMP::HSA: Unable to create context\n";
    abort();
  }
  return context;
}


void RegisterMemory(void *p, size_t sz)
{
    //std::cerr << "registering: ptr " << p << " of size " << sz << "\n";
    GetOrInitHSAContext()->registerArrayMemory(p, sz);
}

} // namespace CLAMP
} // namespace Concurrency

///
/// memory allocator
///
namespace Concurrency {

struct mm_info
{
    void* data;
    int count;
};
class HSAAMPAllocator : public AMPAllocator
{ 
private:
    void PushArg(void *kernel, int idx, std::shared_ptr<void>& data) override {
        PushArgImpl(kernel, idx, sizeof(void*), &mem_info[data.get()].data);
    }
    void regist(int count, void *data, bool hasSrc) override {
        //std::cerr << "HSAAMPAllocator::init()" << std::endl;
        void* p = data;
        if (hasSrc)
            p = aligned_alloc(0x1000, count);
        assert(p);
        CLAMP::RegisterMemory(p, count);
        mem_info[data] = {p, count};
    }
    void append(void *kernel, int idx, std::shared_ptr<void>& mm) override {
        PushArgImpl(kernel, idx, sizeof(void*), &mem_info[mm.get()].data);
    }
    void amp_write(void *data) override {
        auto it = mem_info.find(data);
        mm_info &rw = it->second;
        if (rw.data != data)
            memmove(rw.data, data, rw.count);
    }
    void amp_read(void *data) override {
        auto it = mem_info.find(data);
        mm_info &rw = it->second;
        if (rw.data != data)
            memmove(data, rw.data, rw.count);
    }
    void amp_copy(void *dst, void *src, int n) override {
        auto it = mem_info.find(src);
        mm_info &rw = it->second;
        memmove(dst, rw.data, n);
    }
    void unregist(void *data) override {
        auto it = mem_info.find(data);
        mm_info &rw = it->second;
        if (rw.data != data)
            ::operator delete(rw.data);
        mem_info.erase(it);
    }

    std::map<void *, mm_info> mem_info;
};

static HSAAMPAllocator amp;

HSAAMPAllocator& getHSAAMPAllocator() {
  return amp;
}

} // namespace Concurrency


///
/// kernel compilation / kernel launching
///

extern "C" void *GetAllocatorImpl() {
  return &Concurrency::amp;
}

extern "C" void EnumerateDevicesImpl(int* devices, int* device_number) {
  // FIXME this is a dummy implementation where we always add one GPU device
  // in the future it shall be changed to use hsa_iterate_agents
  if (device_number != nullptr) {
    *device_number = 1;
  }
  if (devices != nullptr) {
    devices[0] = AMP_DEVICE_TYPE_GPU;
  }
}

extern "C" void QueryDeviceInfoImpl(const wchar_t* device_path,
  bool* supports_cpu_shared_memory,
  size_t* dedicated_memory,
  bool* supports_limited_double_precision,
  wchar_t* description) {

  // FIXME this is a somewhat dummy implementation
  const wchar_t des[] = L"HSA";
  wmemcpy(description, des, sizeof(des));
  *supports_cpu_shared_memory = true;
  *supports_limited_double_precision = true;
  *dedicated_memory = 0;
}

static std::map<std::string, HSAContext::Kernel *> __mcw_hsa_kernels;
extern "C" void *CreateKernelImpl(const char* s, void* kernel_size_, void* kernel_source_) {
  std::string str(s);
  HSAContext::Kernel *kernel = __mcw_hsa_kernels[str];
  if (!kernel) {
      size_t kernel_size = (size_t)((void *)kernel_size_);
      char *kernel_source = (char*)malloc(kernel_size+1);
      memcpy(kernel_source, kernel_source_, kernel_size);
      kernel_source[kernel_size] = '\0';
      std::string kname = std::string("&")+s;
      //std::cerr << "CLAMP::HSA::Creating kernel: " << kname << "\n";
      kernel = Concurrency::CLAMP::GetOrInitHSAContext()->
          createKernel(kernel_source, kernel_size, kname.c_str());
      free(kernel_source);
      if (!kernel) {
          std::cerr << "CLAMP::HSA: Unable to create kernel\n";
          abort();
      } else {
          //std::cerr << "CLAMP::HSA: Created kernel\n";
      }
      __mcw_hsa_kernels[str] = kernel;
  }

  HSAContext::Dispatch *dispatch = Concurrency::CLAMP::GetOrInitHSAContext()->createDispatch(kernel);
  dispatch->clearArgs();
#define HSAIL_HLC_DEVELOPMENT_COMPILER 1
#ifndef HSAIL_HLC_DEVELOPMENT_COMPILER
  //dispatch->pushLongArg(0);
  //dispatch->pushLongArg(0);
  //dispatch->pushLongArg(0);
  dispatch->pushLongArg(0);
  dispatch->pushLongArg(0);
  dispatch->pushLongArg(0);
#endif
  return dispatch;
}

extern "C" void LaunchKernelImpl(void *ker, size_t nr_dim, size_t *global, size_t *local)
{
  HSAContext::Dispatch *dispatch =
      reinterpret_cast<HSAContext::Dispatch*>(ker);
  size_t tmp_local[] = {0, 0, 0};
  if (!local)
      local = tmp_local;
  //std::cerr<<"Launching: nr dim = " << nr_dim << "\n";
  //for (size_t i = 0; i < nr_dim; ++i) {
  //  std::cerr << "g: " << global[i] << " l: " << local[i] << "\n";
  //}
  Concurrency::HSAAMPAllocator& aloc = Concurrency::getHSAAMPAllocator();
  dispatch->setLaunchAttributes(nr_dim, global, local);
  //std::cerr << "Now real launch\n";
  dispatch->dispatchKernelWaitComplete();
  delete(dispatch);
}

extern "C" void *LaunchKernelAsyncImpl(void *ker, size_t nr_dim, size_t *global, size_t *local)
{
  HSAContext::Dispatch *dispatch =
      reinterpret_cast<HSAContext::Dispatch*>(ker);
  size_t tmp_local[] = {0, 0, 0};
  if (!local)
      local = tmp_local;

  //std::cerr<<"Launching: nr dim = " << nr_dim << "\n";
  //for (size_t i = 0; i < nr_dim; ++i) {
  //  std::cerr << "g: " << global[i] << " l: " << local[i] << "\n";
  //}
  Concurrency::HSAAMPAllocator& aloc = Concurrency::getHSAAMPAllocator();
  dispatch->setLaunchAttributes(nr_dim, global, local);
  std::shared_future<void>* fut = dispatch->dispatchKernelAndGetFuture();
  return static_cast<void*>(fut);
}

extern "C" void MatchKernelNamesImpl(char *fixed_name) {
  // In HSA kernel names don't need to be fixed
}

extern "C" void PushArgImpl(void *ker, int idx, size_t sz, const void *v) {
  //std::cerr << "pushing:" << ker << " of size " << sz << "\n";
  HSAContext::Dispatch *dispatch =
      reinterpret_cast<HSAContext::Dispatch*>(ker);
  void *val = const_cast<void*>(v);
  switch (sz) {
    case sizeof(double):
      dispatch->pushDoubleArg(*reinterpret_cast<double*>(val));
      break;
    case sizeof(int):
      dispatch->pushIntArg(*reinterpret_cast<int*>(val));
      //std::cerr << "(int) value = " << *reinterpret_cast<int*>(val) <<"\n";
      break;
    case sizeof(unsigned char):
      dispatch->pushBooleanArg(*reinterpret_cast<unsigned char*>(val));
      break;
    default:
      assert(0 && "Unsupported kernel argument size");
  }
}

extern "C" void PushArgPtrImpl(void *ker, int idx, size_t sz, const void *v) {
  //std::cerr << "pushing:" << ker << " of size " << sz << "\n";
  HSAContext::Dispatch *dispatch =
      reinterpret_cast<HSAContext::Dispatch*>(ker);
  void *val = const_cast<void*>(v);
  dispatch->pushPointerArg(val);
}

