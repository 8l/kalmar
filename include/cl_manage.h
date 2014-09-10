//===----------------------------------------------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __CL_MANAGE__
#define __CL_MANAGE__

#pragma once
#include <string.h>
#include <CL/opencl.h>

struct mm_info
{
    cl_mem dm;
    size_t count;
    void *host;
    bool dirty;
    bool discard;
    bool write;
    bool isArray;
    bool isConst;
};

struct AMPAllocator
{
    AMPAllocator() {
        cl_uint          num_platforms;
        cl_int           err;
        cl_platform_id   platform_id[10];
        int i;
        err = clGetPlatformIDs(10, platform_id, &num_platforms);
        for (i = 0; i < num_platforms; i++) {
            // AMD OpenCL 2.0 driver doesn't support CL_DEVICE_TYPE_GPU yet
            //err = clGetDeviceIDs(platform_id[i], CL_DEVICE_TYPE_GPU, 1, &device, NULL);
            err = clGetDeviceIDs(platform_id[i], CL_DEVICE_TYPE_CPU, 1, &device, NULL);
            if (err == CL_SUCCESS)
                break;
        }
        if (err != CL_SUCCESS) {
            for (i = 0; i < num_platforms; i++) {
                err = clGetDeviceIDs(platform_id[i], CL_DEVICE_TYPE_CPU, 1, &device, NULL);
                if (err == CL_SUCCESS)
                    break;
            }
        }
        assert(err == CL_SUCCESS);
        context = clCreateContext(0, 1, &device, NULL, NULL, &err);
        assert(err == CL_SUCCESS);
        queue = clCreateCommandQueueWithProperties(context, device, 0, &err);
        assert(err == CL_SUCCESS);
    }
    void AMPMalloc(void **cpu_ptr, size_t count) {
        cl_int err;

        // use clSVMAlloc to allocate memory
        *cpu_ptr = clSVMAlloc(context, CL_MEM_READ_WRITE, count, 0);

        // use CL_MEM_USE_HOST_PTR for SVM buffer
        cl_mem dm = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, count, *cpu_ptr, &err);
        assert(err == CL_SUCCESS);

        al_info[*cpu_ptr] = {dm, count, *cpu_ptr, false, false, false, false, false};
    }
    void AMPMalloc(void **cpu_ptr, size_t count, void **data_ptr, bool isConst) {
        cl_int err;

        // use clSVMAlloc to allocate memory
        *cpu_ptr = clSVMAlloc(context, CL_MEM_READ_WRITE, count, 0);

        // use CL_MEM_USE_HOST_PTR for SVM buffer
        cl_mem dm = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, count, *cpu_ptr, &err);
        assert(err == CL_SUCCESS);

        memcpy(*cpu_ptr, *data_ptr, count);
        al_info[*cpu_ptr] = {dm, count, *data_ptr, false, false, false, false, isConst};
    }
    void setArray(void *p) {
        al_info[p].isArray = true;
    }
    void refresh(void *p) {
        mm_info mm = al_info[p];
        if (mm.host != p)
            memcpy(p, mm.host, mm.count);
    }
    void synchronize(void *p) {
        mm_info& mm = al_info[p];
        if (mm.host != p && mm.dirty) {
            memcpy(mm.host, p, mm.count);
            mm.dirty = false;
        }
    }
    // discard is an optimization hint to runtime not
    // to copy data to device
    // 1. don't synchronize at destruction time
    // 2. the data is write only, don't need to
    //    copy to device. There is a trick case,
    //    however, if the array_view is projected
    //    from array and discarded, we still need to
    //    copy the data to device.
    void discard(void *p) {
        al_info[p].dirty = false;
        al_info[p].discard = !al_info[p].isArray;
    }
    // can be optimizated in future
    void *getData(void *p) {
        if (al_info[p].dirty)
            return p;
        return al_info[p].host;
    }
    void *getHost(void *p) {
        return al_info[p].host;
    }
    void write() {
        cl_int err;
        for (auto& iter : al_info) {
            mm_info& mm = iter.second;
            if (mm.write) {
                if (!mm.discard) {
                    void *dst = mm.dirty ? iter.first : mm.host;
                    // No longer need clEnqueueWriteBuffer in SVM case
                    //err = clEnqueueWriteBuffer(queue, mm.dm, CL_TRUE, 0,
                    //                           mm.count, dst, 0, NULL, NULL);
                    //assert(err == CL_SUCCESS);
                    mm.discard = false;
                }
                // don't need to read const data back;
                mm.dirty = !mm.isConst;
                mm.write = !mm.isConst;
            }
        }
    }
    void read() {
        cl_int err;
        for (auto& iter : al_info) {
            mm_info& mm = iter.second;
            if (mm.write) {
                // No longer need clEnqueueReadBuffer in SVM case
                //err = clEnqueueReadBuffer(queue, mm.dm, CL_TRUE, 0,
                //                          mm.count, iter.first, 0, NULL, NULL);
                mm.write = false;
                //assert(err == CL_SUCCESS);
            }
        }
    }
    cl_mem getmem(void *p) {
        al_info[p].write = true;
        return al_info[p].dm;
    }
    void AMPFree() {
        for (auto& iter : al_info) {
            mm_info mm = iter.second;
            clReleaseMemObject(mm.dm);
            if (iter.first != mm.host)
                // use SVM free
                clSVMFree(context, iter.first);
        }
        al_info.clear();
    }
    void AMPFree(void *cpu_ptr) {
        mm_info mm = al_info[cpu_ptr];
        if (!mm.discard)
            synchronize(cpu_ptr);
        clReleaseMemObject(mm.dm);
        if (cpu_ptr != mm.host)
            // use SVM free
            clSVMFree(context, cpu_ptr);
        al_info.erase(cpu_ptr);
    }
    ~AMPAllocator() {
        AMPFree();
        clReleaseContext(context);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
    }
    std::map<void *, mm_info>  al_info;
    cl_context       context;
    cl_device_id     device;
    cl_kernel        kernel;
    cl_command_queue queue;
    cl_program       program;
};

AMPAllocator& getAllocator();

template <class T>
struct CLAllocator
{
    T* allocate(unsigned n) {
        T *p = nullptr;
        getAllocator().AMPMalloc((void**)(const_cast<T**>(&p)), n * sizeof(T));
        return p;
    }
    T* allocate(unsigned n, T *ptr, bool isConst = false) {
        T *p = nullptr;
        getAllocator().AMPMalloc((void**)(const_cast<T**>(&p)), n * sizeof(T), (void**)(const_cast<T**>(&ptr)), isConst);
        return p;
    }
};

template <class T>
struct CLDeleter {
    void operator()(T* ptr) {
        getAllocator().AMPFree(ptr);
    }
};

template <class T>
struct ReinDeleter {
    void operator()(T* ptr) {}
};

// Dummy interface that looks somewhat like std::shared_ptr<T>
template <typename T>
class _data {
  typedef typename std::remove_const<T>::type nc_T;
  friend _data<const T>;
  friend _data<nc_T>;
 public:
  _data() = delete;
  _data(const _data& d) restrict(cpu, amp):p_(d.p_) {}
  template <class = typename std::enable_if<std::is_const<T>::value>::type>
    _data(const _data<nc_T>& d) restrict(cpu, amp):p_(d.p_) {}
  template <class = typename std::enable_if<!std::is_const<T>::value>::type>
    _data(const _data<const T>& d) restrict(cpu, amp):p_(const_cast<T*>(d.p_)) {}
  template <typename T2>
    _data(const _data<T2>& d) restrict(cpu, amp):p_(reinterpret_cast<T *>(d.get())) {}
  __attribute__((annotate("user_deserialize")))
  explicit _data(__global T* t) restrict(cpu, amp) { p_ = t; }
  __global T* get(void) const restrict(cpu, amp) { return p_; }
  __global T* get_mutable(void) const restrict(cpu, amp) { return p_; }
  __global T* get_data() const { return get(); }
  void reset(__global T *t = NULL) restrict(cpu, amp) { p_ = t; }
 private:
  __global T* p_;
};


template <typename T>
class _data_host: public std::shared_ptr<T> {
 public:
  _data_host(const _data_host &other):std::shared_ptr<T>(other) {}
  template <class = typename std::enable_if<!std::is_const<T>::value>::type>
  _data_host(const _data_host<const T> &other):std::shared_ptr<T>(const_cast<T *>(other.get_device()), ReinDeleter<T>()) {}
  _data_host(std::nullptr_t x = nullptr):std::shared_ptr<T>(nullptr) {}
  template<class Deleter> _data_host(T* ptr, Deleter d) : std::shared_ptr<T>(ptr, d) {}
  T *get_data() const { return (T *)getAllocator().getData(std::shared_ptr<T>::get()); }
  T *get_device() const { return std::shared_ptr<T>::get(); }
  T *get() const { return (T *)getAllocator().getHost(std::shared_ptr<T>::get()); }

  __attribute__((annotate("serialize")))
  void __cxxamp_serialize(Serialize& s) const {
      cl_mem mm = getAllocator().getmem(std::shared_ptr<T>::get());
      s.Append(sizeof(cl_mem), &mm);
  }
  __attribute__((annotate("user_deserialize")))
  explicit _data_host(__global T* t);
};
#endif // __CL_MANAGE__
