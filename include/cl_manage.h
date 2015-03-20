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

#define CXXAMP_SYNC (1)

#if defined(CXXAMP_NV)
struct rw_info
{
    int count;
    bool used;
    bool discard;
    bool ready_to_read;
    cl_mem dm;
};
#endif
struct amp_obj
{
    cl_mem dm;
    int count;
};

#define QUEUE_SIZE (1)

struct DimMaxSize {
  cl_uint dimensions;
  size_t* maxSizes;
};
extern std::map<cl_device_id, struct DimMaxSize> Clid2DimSizeMap;
namespace CLAMP {
extern void AddKernelEventObject(cl_kernel, cl_event);
extern std::vector<cl_event>& GetKernelEventObject(cl_kernel);
extern void RemoveKernelEventObject(cl_kernel);
}
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

struct AMPAllocator
{
    inline cl_command_queue getQueue() {
        //printf("use queue %d\n", queue_id);
        cl_command_queue ret = queue[queue_id];
        queue_id = (queue_id + 1) % QUEUE_SIZE;
        return ret;
    }

    AMPAllocator(cl_device_id a_device, std::wstring a_device_path)
      : device(a_device), program(NULL), m_maxCommandQueuePerDevice(QUEUE_SIZE) {
        int i;
        cl_int           err;
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

      resetLock();
      _accelerator = accelerator(a_device_path);
      assert (_accelerator.get_device_id() == a_device);
    }
    void init(void *data, int count) {
        auto iter = mem_info.find(data);
        if (iter == std::end(mem_info)) {
            if (count > 0) {
                cl_int err;
#if defined(CXXAMP_NV)
                cl_mem dm = clCreateBuffer(context, CL_MEM_READ_WRITE, count, NULL, &err);
                rwq[data] = {count, false, false, false, NULL};
#else
                cl_mem dm = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, count, data, &err);
#endif
                assert(err == CL_SUCCESS);
                mem_info[data] = {dm, 1};
                #if SYNC_DEBUG
                printf("init data=%p, device=%p, count=%d\n", data, dm, count);
                #endif
            }
        } else {
            #if SYNC_DEBUG
            printf("retain data=%p, device=%p, count=%d\n", data, iter->second.dm,count);
            #endif
            ++iter->second.count;
          }
    }
    void append(Serialize& s, void *data) {
        s.Append(sizeof(cl_mem), &mem_info[data].dm);
#if defined(CXXAMP_NV)
        rwq[data].used = true;
#endif
    }
#if defined(CXXAMP_NV)
    std::map<void *, rw_info>& synchronize(){
      return rwq;
    }
    
    void discard(void* data) {
      auto it = rwq.find(data);
      if (it != std::end(rwq)) {
        #ifdef SYNC_DEBUG
        printf("discard data %p\n", data);
        #endif
        it->second.discard = true;
      }
    }
    void write() {
      cl_int err;
      for (auto& it : rwq) {
        rw_info& rw = it.second;
        // Do sync under the following compound condition
        // (1) if serialized
        // (2) if not dicarded by users
        // (3) if device data is still valid
        if (rw.used && !rw.discard && !rw.ready_to_read) {
          #ifdef SYNC_DEBUG
          printf("sync write from host %p, device %p\n", it.first, mem_info[it.first].dm);
          #endif
          err = clEnqueueWriteBuffer(getQueue(), mem_info[it.first].dm, CL_TRUE, 0,
                                     rw.count, it.first, 0, NULL, NULL);
          assert(err == CL_SUCCESS);
        }
      }
    }
    // Used in Concurrency::copy
    void* device_data(void* data) {
      if (!data)
        return NULL;
      auto it = rwq.find(data);
      if (it != std::end(rwq)) {
        return mem_info[data].dm;
      }
      return NULL;
    }
    // Determine if the device_ptr is allocated on assocated accelerator or not
    bool isAllocated(void* device_ptr) {
      if (!device_ptr)
        return false;
      for (auto &it: rwq) {
        // Reverse lookup of the map
        if(device_ptr == mem_info[it.first].dm)
          return true;
      }
      return false;
    }
    void read() {
      for (auto& it : rwq) {
        rw_info& rw = it.second;
        if (rw.used) {
          #ifdef SYNC_DEBUG
          printf("need read host %p, device %p\n", it.first, mem_info[it.first].dm);
          #endif
          rw.ready_to_read = true;
          rw.dm = mem_info[it.first].dm;
        }
      }
    }
#endif
    void free(void *data) {
      auto iter = mem_info.find(data);
      if (iter != std::end(mem_info) && --iter->second.count == 0) {
        #ifdef SYNC_DEBUG
        printf("Release mem data=%p, dm=%p\n", data, iter->second.dm);
        #endif
        clReleaseMemObject(iter->second.dm);
        mem_info.erase(iter);
        #if CXXAMP_NV
        auto it = rwq.find(data);
        if (it != std::end(rwq))
          rwq.erase(it);
        #endif
      }
    }
    ~AMPAllocator() {
        if (program)
          clReleaseProgram(program);
        for (int i = 0; i < QUEUE_SIZE; ++i) {
          clReleaseCommandQueue(queue[i]);
        }
        clReleaseContext(context);
    }
    bool tryLock() {
      int old_val = m_atomicLock.fetch_add();
      if ((old_val+1) <= m_maxCommandQueuePerDevice) {
        return true;
      } else {
        m_atomicLock.fetch_sub();
        return false;
      }
    }
    void releaseLock()
    {
      int old_val = m_atomicLock.fetch_sub();
      assert(old_val >= 1);
    };
    void resetLock()
    {
      (m_atomicLock.get()).store(0); 
    };
    Concurrency::accelerator* get_accelerator() {
      return &_accelerator;
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
    AtomCopyable<int> m_atomicLock;
    int m_maxCommandQueuePerDevice;
    Concurrency::accelerator _accelerator;
};

AMPAllocator* getAllocator(cl_device_id id);
#if defined(CXXAMP_NV)
void* getDevicePointer(void* data);
cl_command_queue getOCLQueue(void* device_ptr);
#endif
struct mm_info
{
    std::vector<cl_kernel> serializedKernel;
    void *data;
    bool discard;
    bool sync;
    bool free;
    cl_device_id _device_id;
    //static int waitOnKernelsCount;
    mm_info(int count, cl_device_id device_id)
      : data(aligned_alloc(0x1000, count)), discard(false), free(true) {
      assert(device_id);
      _device_id = device_id;
      getAllocator(_device_id)->init(data, count);
    }
    mm_info(int count, void *src, cl_device_id device_id)
      : data(src), discard(false), free(false) {
      assert(device_id);
      _device_id = device_id;
      getAllocator(_device_id)->init(data, count);
    }
    void synchronize() {
      #if CXXAMP_NV
      std::map<void *, rw_info> &rwq = getAllocator(_device_id)->synchronize();
      {
        cl_int err;
        for (auto& it : rwq) {
          rw_info& rw = it.second;
          // Do sync under the following compound condition
          // (1) if serialized
          // (2) if device data is validated
          // (3) if is the interested host pointer
          if (rw.used && rw.ready_to_read && it.first == data) {
            #ifdef SYNC_DEBUG
            printf("sync read back to host=%p, device=%p, count=%d\n\n",data, rw.dm, rw.count);
            #endif
            err = clEnqueueReadBuffer(getAllocator(_device_id)->getQueue(), rw.dm, CL_TRUE, 0,
                                      rw.count, data, 0, NULL, NULL);
            assert(err == CL_SUCCESS);
            rw.used = false;
            rw.ready_to_read = false;
          }
        }
      }
      #endif
      //printf("mm_info::synchronize() : %d\n", ++waitOnKernelsCount);
      waitOnKernels();
    }
    void refresh() {}
    void* get() { return data; }
    void disc() {
      discard = true;
      #if CXXAMP_NV
      getAllocator(_device_id)->discard(data);
      #endif
    }
    void serialize(Serialize& s) {
      #ifdef SYNC_DEBUG
      printf("serialize, data=%p\n",data);
      #endif
      #if !CXXAMP_SYNC
      serializedKernel.push_back(s.getKernel());
      #endif
      getAllocator(_device_id)->append(s, data);
    }
    ~mm_info() {
        //printf("mm_info::~mm_info() : %d\n", ++waitOnKernelsCount);
      waitOnKernels();
      if (!discard)
        synchronize();
      getAllocator(_device_id)->free(data);
      //free = true;
      if (0) {
        ::operator delete(data);
      }
    }
    void waitOnKernels() {
        #if !CXXAMP_SYNC
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
       #endif
    }
};

// Dummy interface that looks somewhat like std::shared_ptr<T>
template <typename T>
class _data {
public:
    _data() = delete;
    _data(int count, cl_device_id device_id) : _device_id(device_id) {}
    _data(const _data& d) restrict(cpu, amp)
        : p_(d.p_), _device_id(d._device_id) {}
    template <typename U>
        _data(const _data<U>& d) restrict(cpu, amp)
        : p_(reinterpret_cast<T *>(d.get())), _device_id(d.device_id) {}
    __attribute__((annotate("user_deserialize")))
        explicit _data(__global T* t) restrict(cpu, amp) { p_ = t; }
    __global T* get(void) const restrict(cpu, amp) { return p_; }
    cl_device_id device_id() const { return _device_id; }
private:
    __global T* p_;
    cl_device_id _device_id; // Meaningless. The only reason is to make compilation happy
};

template <typename T>
class _data_host {
    std::shared_ptr<mm_info> mm;
    template <typename U> friend class _data_host;
public:
    _data_host(int count, cl_device_id device_id)
        : mm(std::make_shared<mm_info>(count * sizeof(T), device_id)) {}
    _data_host(int count, T* src, cl_device_id device_id)
        : mm(std::make_shared<mm_info>(count * sizeof(T), src, device_id)) {}
    _data_host(const _data_host& other)
        : mm(other.mm) {}
    template <typename U>
        _data_host(const _data_host<U>& other) : mm(other.mm) {}

    T *get() const { return (T *)mm->get(); }
    void synchronize() const { mm->synchronize(); }
    void discard() const { mm->disc(); }
    void refresh() const { mm->refresh(); }
    cl_device_id device_id() const { return mm->_device_id; }
    __attribute__((annotate("serialize")))
        void __cxxamp_serialize(Serialize& s) const {
            mm->serialize(s);
        }
    __attribute__((annotate("user_deserialize")))
        explicit _data_host(__global T* t);
};
#endif // __CL_MANAGE__
