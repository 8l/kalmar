//===----------------------------------------------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cassert>
#include <exception>
#include <string>
#include <vector>
#include <map>
#include <string.h> //memcpy
#if !defined(CXXAMP_ENABLE_HSA)
#include <CL/opencl.h>
#include <cl_manage.h>
#endif
#include <memory>
#include <atomic> // atmotic lock

/* COMPATIBILITY LAYER */
#define STD__FUTURE_STATUS__FUTURE_STATUS std::future_status

#ifndef WIN32
#define __declspec(ignored) /* */
#endif

namespace Concurrency
{
namespace CLAMP {
extern void ReleaseKernelObject();
}
namespace details
{
// GPU device management
class DeviceManager
{
public:
  DeviceManager() {
    cl_uint num_platforms;
    starting_id = NULL;
    cl_int err;
    cl_device_id devices[1024];
    int gpuCount = 0;
    cl_uint num_devices;
    std::unique_ptr<cl_platform_id[]> platforms;
    err = clGetPlatformIDs(0, NULL, &num_platforms);
    assert(err == CL_SUCCESS);
    platforms.reset(new cl_platform_id[num_platforms]);
    clGetPlatformIDs(num_platforms, platforms.get(), NULL);
    assert(err == CL_SUCCESS);
    for (int i = 0; i < num_platforms; i++) {
      // FIXME: need to handle if there are multiple platforms
      err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 1024, devices, &num_devices);
      if (err == CL_SUCCESS) {
        for (int j = 0; j < num_devices; j++) {
          if (!starting_id) starting_id = devices[j];
          gpuCount++;
          std::wstring path = L"gpu" + std::to_wstring(j);
          m_allocators.insert(std::pair<cl_device_id, AMPAllocator*>(
                           devices[j], new Concurrency::AMPAllocator(devices[j], path)));
          m_ids.insert(std::pair<int, cl_device_id>(j, devices[j]));
        }
      }
    }
    assert(gpuCount > 0);

    m_count = gpuCount;
  }

  ~DeviceManager() {
    CLAMP::ReleaseKernelObject();
    for(const auto& it : Clid2DimSizeMap)
     if(it.second.maxSizes)
      delete[] it.second.maxSizes;

    for (auto& it : m_allocators) {
      delete it.second;
    }
  }
  // Get device count
  int getCount() const {return m_count;}

  static cl_device_id starting_device() {
    return starting_id;
  }
  AMPAllocator* getAllocator(cl_device_id id) {
    auto iter = m_allocators.find(id);
    if (iter != std::end(m_allocators)) {
      return iter->second;
    } else {
      printf("No such device, id = %p, exit\n", id);
      exit(1);
    }
  }
#if defined(CXXAMP_NV)
  void* getDevicePointer(void* data) {
    for (auto &it: m_allocators) {
      AMPAllocator* amp = it.second;
      void* devicePtr = amp->device_data(data);
      if (devicePtr)
        return devicePtr;
    }
    return NULL;
  }
  cl_command_queue getOCLQueue(void* device_ptr) {
    for (auto &it: m_allocators) {
      AMPAllocator* amp = it.second;
      if(amp && amp->isAllocated(device_ptr))
        return amp->getQueue();
    }
    return NULL;
  }
#endif
  // TODO: shall returned pre-created accelerators
  cl_device_id getAvailableDevice()
  {
    cl_device_id did = NULL;
    // Blocking: if none is available at this moment, spin until we get one
    bool avail = false;
    do {
      for (auto &it: m_allocators) {
        avail = it.second->tryLock();
        if (avail) {
          did = it.first;
          break;
        }
      }
    } while(!avail);
    
    assert (did != NULL);
    return did;
  }

private:
  int m_count;
  std::map<cl_device_id, AMPAllocator*> m_allocators;
  std::map<int, cl_device_id> m_ids;

public:
  static cl_device_id starting_id; // the very first GPU device
};
}; // namespace details
}; // namespace Concurrency::details
