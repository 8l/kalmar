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
namespace details
{
// wrapper for std::atom to enable copy assign
template <typename T>
struct AtomCopyable
{
  std::atomic<T> _impl;
  AtomCopyable() :_impl() {}
  AtomCopyable(const std::atomic<T> &other) :_impl(other.load()) {}
  AtomCopyable(const AtomCopyable &other) :_impl(other._impl.load()) {}
  AtomCopyable &operator=(const AtomCopyable &other) {
    _impl.store(other._impl.load());
  }
  // llvm.org/viewvc/llvm-project?view=revision&revision=183033
  T fetch_add() { return _impl.fetch_add(1); };
  T fetch_sub() { return _impl.fetch_sub(1); };
  std::atomic<T> &get() { return _impl; }
};

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
          m_allocators.insert(std::pair<cl_device_id, AMPAllocator*>(
                           devices[j], new Concurrency::AMPAllocator(devices[j])));
          m_ids.insert(std::pair<int, cl_device_id>(j, devices[j]));
        }
      }
    }
    assert(gpuCount > 0);

    m_count = gpuCount;
    m_atomicLock.resize(gpuCount);
    m_maxCommandQueuePerDevice.resize(gpuCount);
    for (int i = 0; i < gpuCount; i++) {
      resetLock(i);
      m_maxCommandQueuePerDevice[i] = 2;
    }
  }

  ~DeviceManager() {
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

  bool tryLock(int deviceIndex)
  {
    assert(deviceIndex >= 0);
    int leftover = m_atomicLock[deviceIndex].fetch_add();
    if (leftover < m_maxCommandQueuePerDevice[deviceIndex]) {
      return true;
    } else {
      m_atomicLock[deviceIndex].fetch_sub();
      return false;
    }
  }

  void releaseLock(int deviceIndex) 
  {
    int leftover = m_atomicLock[deviceIndex].fetch_sub();
    assert(leftover >= 1);
  };

  void resetLock(int deviceIndex) 
  {
    (m_atomicLock[deviceIndex].get()).store(0); 
  };

  // TODO: shall returned pre-created accelerators
  cl_device_id getAvailableDevice()
  {
    int device_index = -1;
    // Poll to see if any device is available.
    for (unsigned i=0; i < getCount(); i++) {
      bool avail = tryLock(i);
      if (avail) {
        device_index = i;
        break;
      }
    }
    if (device_index == -1) {
      // Blocking: if none is available at this moment, spin until we get one
      bool avail = false;
      do {
        for (unsigned i=0; i < getCount(); i++) {
          avail = tryLock(i);
          if (avail) {
            device_index = i;
            break;
          }
        }
      } while(!avail);
    }
    assert (device_index != -1);

    return m_ids[device_index];
  }

private:
  int m_count;
  std::map<cl_device_id, AMPAllocator*> m_allocators;
  std::map<int, cl_device_id> m_ids;
  std::vector< AtomCopyable<int> > m_atomicLock;
  std::vector<int> m_maxCommandQueuePerDevice;

public:
  static cl_device_id starting_id;
};
}; // namespace details
}; // namespace Concurrency::details
