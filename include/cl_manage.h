//===----------------------------------------------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __CL_MANAGE__
#define __CL_MANAGE__

#pragma once
#include <stdlib.h>
#include <string.h>
#include <CL/opencl.h>


#if defined(CXXAMP_NV)
struct rw_info
{
    int count;
    bool used;
};
#endif
struct amp_obj
{
    cl_mem dm;
    int count;
};

#define QUEUE_SIZE (2)

struct DimMaxSize {
  cl_uint dimensions;
  size_t* maxSizes;
};
extern std::map<cl_device_id, struct DimMaxSize> Clid2DimSizeMap;
namespace CLAMP {
extern void ReleaseKernelObject();
extern void AddKernelEventObject(cl_kernel, cl_event);
extern std::vector<cl_event>& GetKernelEventObject(cl_kernel);
extern void RemoveKernelEventObject(cl_kernel);
}

struct AMPAllocator
{
    inline cl_command_queue getQueue() {
        //printf("use queue %d\n", queue_id);
        cl_command_queue ret = queue[queue_id];
        queue_id = (queue_id + 1) % QUEUE_SIZE;
        return ret;
    }

    AMPAllocator() {
        cl_uint          num_platforms;
        cl_int           err;
        cl_platform_id   platform_id[10];
        int i;
        err = clGetPlatformIDs(10, platform_id, &num_platforms);
        for (i = 0; i < num_platforms; i++) {
            err = clGetDeviceIDs(platform_id[i], CL_DEVICE_TYPE_GPU, 1, &device, NULL);
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
        for (i = 0; i < QUEUE_SIZE; ++i) {
          queue[i] = clCreateCommandQueue(context, device, 0, &err);
          assert(err == CL_SUCCESS);
        }
        queue_id = 0;
        // Propel underlying OpenCL driver to enque kernels faster (pthread-based)
        // FIMXE: workable on AMD platforms only
        pthread_t self = pthread_self();
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        int result = -1;
        // Get max priority
        int policy = 0;
        result = pthread_attr_getschedpolicy(&attr, &policy);
        if (result != 0)
          perror("getsched error!\n");
        int max_prio = sched_get_priority_max(policy);

        struct sched_param param;
        // Get self priority
        result = pthread_getschedparam(self, &policy, &param);
        if (result != 0)
          perror("getsched self error!\n");
        int self_prio = param.sched_priority;
        #if 0
        printf("self=%d, self_prio = %d,  max = %d\n", (int)self, self_prio, max_prio);
        #endif
        for (int qid = 0; qid < QUEUE_SIZE; qid++) {
          #define CL_QUEUE_THREAD_HANDLE_AMD 0x403E
          #define PRIORITY_OFFSET 2
          void* handle=NULL;
          cl_int status = clGetCommandQueueInfo (queue[qid], CL_QUEUE_THREAD_HANDLE_AMD, sizeof(handle), &handle, NULL );
          // Ensure it is valid
          if (status == CL_SUCCESS && handle) {
            pthread_t thId = (pthread_t)handle;
            result = pthread_getschedparam(thId, &policy, &param);
            if (result != 0)
              perror("getsched q error!\n");
            int que_prio = param.sched_priority;
            #if 0
            printf("que=%d, que_prio = %d\n", (int)thId, que_prio);
            #endif
            // Strategy to renew the que thread's priority, the smaller the highest
            if (max_prio == que_prio) {
              // perfect. Do nothing
              continue;
            } else if (max_prio < que_prio && que_prio <= self_prio) {
              // self    que    max
              que_prio = (que_prio-PRIORITY_OFFSET)>0?(que_prio-PRIORITY_OFFSET):que_prio;
            } else if (que_prio > self_prio) {
              // que   self    max
              que_prio = (self_prio-PRIORITY_OFFSET)>0?(self_prio-PRIORITY_OFFSET):self_prio;
            } 
            int result = pthread_setschedprio(thId, que_prio);
            if (result != 0)
              perror("Renew p error!\n");
          } 
        }
        pthread_attr_destroy(&attr);

      // C++ AMP specifications
      // The maximum number of tiles per dimension will be no less than 65535.
      // The maximum number of threads in a tile will be no less than 1024.
      // In 3D tiling, the maximal value of D0 will be no less than 64.
      cl_uint dimensions = 0;
      err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(cl_uint), &dimensions, NULL);
      assert(err == CL_SUCCESS);
      size_t* maxSizes = new size_t[dimensions];
      err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t) * dimensions, maxSizes, NULL);
      assert(err == CL_SUCCESS);
      struct DimMaxSize d;
      d.dimensions = dimensions;
      d.maxSizes = maxSizes;
      Clid2DimSizeMap[device] = d;
    }
    void init(void *data, int count) {
        auto iter = mem_info.find(data);
        if (iter == std::end(mem_info)) {
            if (count > 0) {
                cl_int err;
#if defined(CXXAMP_NV)
                cl_mem dm = clCreateBuffer(context, CL_MEM_READ_WRITE, count, NULL, &err);
                rwq[data] = {count, false};
#else
                cl_mem dm = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, count, data, &err);
#endif
                assert(err == CL_SUCCESS);
                mem_info[data] = {dm, 1};
            }
        } else
            ++iter->second.count;
    }
    void append(Serialize& s, void *data) {
        s.Append(sizeof(cl_mem), &mem_info[data].dm);
#if defined(CXXAMP_NV)
        rwq[data].used = true;
#endif
    }
#if defined(CXXAMP_NV)
    void write() {
        cl_int err;
        for (auto& it : rwq) {
            rw_info& rw = it.second;
            if (rw.used) {
                err = clEnqueueWriteBuffer(getQueue(), mem_info[it.first].dm, CL_TRUE, 0,
                                           rw.count, it.first, 0, NULL, NULL);
                assert(err == CL_SUCCESS);
            }
        }
    }
    void read() {
        cl_int err;
        for (auto& it : rwq) {
            rw_info& rw = it.second;
            if (rw.used) {
                err = clEnqueueReadBuffer(getQueue(), mem_info[it.first].dm, CL_TRUE, 0,
                                          rw.count, it.first, 0, NULL, NULL);
                assert(err == CL_SUCCESS);
                rw.used = false;
            }
        }
    }
#endif
    void free(void *data) {
        auto iter = mem_info.find(data);
        if (iter != std::end(mem_info) && --iter->second.count == 0) {
            clReleaseMemObject(iter->second.dm);
            mem_info.erase(iter);
        }
    }
    ~AMPAllocator() {
        clReleaseProgram(program);
        for (int i = 0; i < QUEUE_SIZE; ++i) {
          clReleaseCommandQueue(queue[i]);
        }
        clReleaseContext(context);
        for(const auto& it : Clid2DimSizeMap)
          if(it.second.maxSizes)
            delete[] it.second.maxSizes;
        // Release all kernel objects associated with 'program'
        CLAMP::ReleaseKernelObject();
    }
    std::map<void *, amp_obj> mem_info;
    cl_context       context;
    cl_device_id     device;
    cl_command_queue queue[QUEUE_SIZE];
    int              queue_id;
    cl_program       program;
#if defined(CXXAMP_NV)
    std::map<void *, rw_info> rwq;
#endif
};

AMPAllocator& getAllocator();

struct mm_info
{
    std::vector<cl_kernel> serializedKernel;
    void *data;
    bool free;
    //static int waitOnKernelsCount;
    mm_info(int count)
        : data(aligned_alloc(0x1000, count)), free(true) { getAllocator().init(data, count); }
    mm_info(int count, void *src)
        : data(src), free(false) { getAllocator().init(data, count); }
    void synchronize() {
        //printf("mm_info::synchronize() : %d\n", ++waitOnKernelsCount);
        waitOnKernels();
    }
    void refresh() {}
    void* get() { return data; }
    void disc() {}
    void serialize(Serialize& s) {
        serializedKernel.push_back(s.getKernel());
        getAllocator().append(s, data);
    }
    ~mm_info() {
        //printf("mm_info::~mm_info() : %d\n", ++waitOnKernelsCount);
        waitOnKernels();
        getAllocator().free(data);
        if (free)
            ::operator delete(data);
    }
    void waitOnKernels() {
        // for each kernel, check if it has been finished
        std::for_each(serializedKernel.begin(), serializedKernel.end(), [](cl_kernel& k) {
            // get cl_event associated with the kernel
            std::vector<cl_event>& event_vector = CLAMP::GetKernelEventObject(k);
            std::for_each(event_vector.begin(), event_vector.end(), [](cl_event& event) {
                // wait for the event to be completed
                clWaitForEvents(1, &event);

                clReleaseEvent(event);
            });

            // wait done, can remove the event object
            CLAMP::RemoveKernelEventObject(k);
        });
    }
};

// Dummy interface that looks somewhat like std::shared_ptr<T>
template <typename T>
class _data {
public:
    _data() = delete;
    _data(int count) {}
    _data(const _data& d) restrict(cpu, amp)
        : p_(d.p_) {}
    template <typename U>
        _data(const _data<U>& d) restrict(cpu, amp)
        : p_(reinterpret_cast<T *>(d.get())) {}
    __attribute__((annotate("user_deserialize")))
        explicit _data(__global T* t) restrict(cpu, amp) { p_ = t; }
    __global T* get(void) const restrict(cpu, amp) { return p_; }
private:
    __global T* p_;
};

template <typename T>
class _data_host {
    std::shared_ptr<mm_info> mm;
    template <typename U> friend class _data_host;
public:
    _data_host(int count)
        : mm(std::make_shared<mm_info>(count * sizeof(T))) {}
    _data_host(int count, T* src)
        : mm(std::make_shared<mm_info>(count * sizeof(T), src)) {}
    _data_host(const _data_host& other)
        : mm(other.mm) {}
    template <typename U>
        _data_host(const _data_host<U>& other) : mm(other.mm) {}

    T *get() const { return (T *)mm->get(); }
    void synchronize() const { mm->synchronize(); }
    void discard() const { mm->disc(); }
    void refresh() const { mm->refresh(); }

    __attribute__((annotate("serialize")))
        void __cxxamp_serialize(Serialize& s) const {
            mm->serialize(s);
        }
    __attribute__((annotate("user_deserialize")))
        explicit _data_host(__global T* t);
};
#endif // __CL_MANAGE__
