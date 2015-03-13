//===----------------------------------------------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#pragma once
#include <cassert>
#include <future>
#include <utility>
#include <amp.h>
#include <amprt.h>

#if !defined(CXXAMP_ENABLE_HSA)
#include <cl_manage.h>
#endif

namespace Concurrency {
namespace CLAMP {
#if defined(CXXAMP_ENABLE_HSA)
extern void *CreateHSAKernel(std::string);
extern void HSALaunchKernel(void *ker, size_t, size_t *global, size_t *local);
extern std::future<void> HSALaunchKernelAsync(void *ker, size_t, size_t *global, size_t *local);
#endif
extern void MatchKernelNames( std::string & );
extern void CompileKernels(cl_program& program, cl_context& context, cl_device_id& device);
extern cl_kernel GetKernelObject(cl_program& prog, std::string& name);
extern void AddKernelEventObject(cl_kernel, cl_event);
}
static inline std::string mcw_cxxamp_fixnames(char *f) restrict(cpu) {
    std::string s(f);
    std::string out;

    for(std::string::iterator it = s.begin(); it != s.end(); it++ ) {
      if (*it == '_' && it == s.begin()) {
        continue;
      } else if (isalnum(*it) || (*it == '_')) {
        out.append(1, *it);
      } else if (*it == '$') {
        out.append("_EC_");
      }
    }
    //CLAMP::MatchKernelNames(out);
    return out;
}

template<typename Kernel, int dim_ext>
static inline std::future<void> mcw_cxxamp_launch_kernel_async(size_t *ext,
  size_t *local_size, const Kernel& f) restrict(cpu,amp) {
#ifndef __GPU__
#if defined(CXXAMP_ENABLE_HSA)
  //Invoke Kernel::__cxxamp_trampoline as an HSAkernel
  //to ensure functor has right operator() defined
  //this triggers the trampoline code being emitted
  // FIXME: implicitly casting to avoid pointer to int error
  int* foo = reinterpret_cast<int*>(&Kernel::__cxxamp_trampoline);
  void *kernel = NULL;
  {
      std::string transformed_kernel_name =
          mcw_cxxamp_fixnames(f.__cxxamp_trampoline_name());
      kernel = CLAMP::CreateHSAKernel(transformed_kernel_name);
  }
  Concurrency::Serialize s(kernel);
  f.__cxxamp_serialize(s);
  return CLAMP::HSALaunchKernelAsync(kernel, dim_ext, ext, local_size);
#else
  // async kernel launch is unsupported in non-HSA path
  throw runtime_exception("async_parallel_for_each is unsupported on this platform", 0);
#endif
#endif
}

template<typename Kernel, int dim_ext>
static inline void mcw_cxxamp_launch_kernel(size_t *ext,
  size_t *local_size, const Kernel& f,
  const accelerator_view& accl_view) restrict(cpu,amp) {
#ifndef __GPU__
#if defined(CXXAMP_ENABLE_HSA)
  //Invoke Kernel::__cxxamp_trampoline as an HSAkernel
  //to ensure functor has right operator() defined
  //this triggers the trampoline code being emitted
  // FIXME: implicitly casting to avoid pointer to int error
  int* foo = reinterpret_cast<int*>(&Kernel::__cxxamp_trampoline);
  void *kernel = NULL;
  {
      std::string transformed_kernel_name =
          mcw_cxxamp_fixnames(f.__cxxamp_trampoline_name());
      kernel = CLAMP::CreateHSAKernel(transformed_kernel_name);
  }
  Concurrency::Serialize s(kernel);
  f.__cxxamp_serialize(s);
  CLAMP::HSALaunchKernel(kernel, dim_ext, ext, local_size);
#else
  cl_int err;
  // TODO: all captured array_views and arrays shall be located on the same accelerator
  AMPAllocator* aloc = getAllocator(accl_view.get_accelerator().get_device_id());
  CLAMP::CompileKernels(getCLProgram(), aloc->context, aloc->device);
  int* foo = reinterpret_cast<int*>(&Kernel::__cxxamp_trampoline);
  std::string transformed_kernel_name =
      mcw_cxxamp_fixnames(f.__cxxamp_trampoline_name());
  cl_kernel kernel = CLAMP::GetKernelObject(getCLProgram(), transformed_kernel_name);
  Concurrency::Serialize s(kernel);
  f.__cxxamp_serialize(s);
  {
      // C++ AMP specifications
      // The maximum number of tiles per dimension will be no less than 65535.
      // The maximum number of threads in a tile will be no less than 1024.
      // In 3D tiling, the maximal value of D0 will be no less than 64.
      size_t *maxSizes = Clid2DimSizeMap[aloc->device].maxSizes;
      bool is = true;
      int threads_per_tile = 1;
      for(int i = 0; local_size && i < dim_ext; i++) {
          threads_per_tile *= local_size[i];
          // For the following cases, set local_size=NULL and let OpenCL driver arranges it instead
          //(1) tils number exceeds CL_DEVICE_MAX_WORK_ITEM_SIZES per dimension
          //(2) threads in a tile exceeds CL_DEVICE_MAX_WORK_ITEM_SIZES
          //Note that the driver can still handle unregular tile_dim, e.g. tile_dim is undivisble by 2
          //So skip this condition ((local_size[i]!=1) && (local_size[i] & 1))
          if(local_size[i] > maxSizes[i] || threads_per_tile > maxSizes[i]) {
              is = false;
              break;
          }
      }
      if(!is)
          local_size = NULL;
  }

#if defined(CXXAMP_NV)
  aloc->write();
#endif
  cl_event kn_event;
  cl_command_queue queue = aloc->getQueue();
  err = clEnqueueNDRangeKernel( queue, kernel, dim_ext, NULL, ext, local_size, 0, NULL, &kn_event);
  assert(err == CL_SUCCESS);
  #if !CXXAMP_SYNC 
  CLAMP::AddKernelEventObject(kernel, kn_event);
  #endif

  #if CXXAMP_SYNC 
  // Provided the queued kernels are independent, we could flush them
  // We should consider to use multiple queues
  err = clFlush( queue );
  {
    cl_int status = CL_QUEUED;
    while(status != CL_COMPLETE) {
      err = clGetEventInfo(kn_event, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(cl_int),&status,NULL);
      assert(err == CL_SUCCESS);
    }
  }
  #endif
  // Release device lock if specified accelerator_view is automatically selected
  if (accl_view. get_is_auto_selection())
    aloc->releaseLock();

  #if defined(CXXAMP_NV)
  aloc->read();
  #endif
#endif //CXXAMP_ENABLE_HSA
#endif // __GPU__
}

template <int N, typename Kernel, typename _Tp>
struct pfe_helper
{
    static inline void call(Kernel& k, _Tp& idx) restrict(amp,cpu) {
        int i;
        for (i = 0; i < k.ext[N - 1]; ++i) {
            idx[N - 1] = i;
            pfe_helper<N - 1, Kernel, _Tp>::call(k, idx);
        }
    }
};
template <typename Kernel, typename _Tp>
struct pfe_helper<0, Kernel, _Tp>
{
    static inline void call(Kernel& k, _Tp& idx) restrict(amp,cpu) {
        k.k(idx);
    }
};

template <int N, typename Kernel>
class pfe_wrapper
{
public:
    explicit pfe_wrapper(extent<N>& other, const Kernel& f) restrict(amp,cpu)
        : ext(other), k(f) {}
    void operator() (index<N> idx) restrict(amp,cpu) {
        pfe_helper<N - 3, pfe_wrapper<N, Kernel>, index<N>>::call(*this, idx);
    }
private:
    const extent<N> ext;
    const Kernel k;
    template <int K, typename Ker, typename _Tp>
        friend struct pfe_helper;
};

template <int N, typename Kernel>
__attribute__((noinline,used)) void parallel_for_each(
    extent<N> compute_domain, const Kernel& f) restrict(cpu, amp) {
#ifndef __GPU__
    size_t compute_domain_size = 1;
    for(int i = 0 ; i < N ; i++)
    {
      if(compute_domain[i]<=0)
        throw invalid_compute_domain("Extent is less or equal than 0.");
      if (static_cast<size_t>(compute_domain[i]) > 4294967295L)
        throw invalid_compute_domain("Extent size too large.");
      compute_domain_size *= static_cast<size_t>(compute_domain[i]);
      if (compute_domain_size > 4294967295L)
        throw invalid_compute_domain("Extent size too large.");
    }

    size_t ext[3] = {static_cast<size_t>(compute_domain[N - 1]),
        static_cast<size_t>(compute_domain[N - 2]),
        static_cast<size_t>(compute_domain[N - 3])};
    const pfe_wrapper<N, Kernel> _pf(compute_domain, f);
    accelerator_view accl_view = accelerator::get_auto_selection_view();
    mcw_cxxamp_launch_kernel<pfe_wrapper<N, Kernel>, 3>(ext, NULL, _pf, accl_view);
#else
    auto bar = &pfe_wrapper<N, Kernel>::operator();
    auto qq = &index<N>::__cxxamp_opencl_index;
    int* foo = reinterpret_cast<int*>(&pfe_wrapper<N, Kernel>::__cxxamp_trampoline);
#endif
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"
//ND async_parallel_for_each, nontiled
template <int N, typename Kernel>
__attribute__((noinline,used)) completion_future async_parallel_for_each(
    extent<N> compute_domain, const Kernel& f) restrict(cpu,amp) {
#ifndef __GPU__
    size_t compute_domain_size = 1;
    for(int i = 0 ; i < N ; i++)
    {
      if(compute_domain[i]<=0)
        throw invalid_compute_domain("Extent is less or equal than 0.");
      if (static_cast<size_t>(compute_domain[i]) > 4294967295L)
        throw invalid_compute_domain("Extent size too large.");
      compute_domain_size *= static_cast<size_t>(compute_domain[i]);
      if (compute_domain_size > 4294967295L)
        throw invalid_compute_domain("Extent size too large.");
    }

    size_t ext[3] = {static_cast<size_t>(compute_domain[N - 1]),
        static_cast<size_t>(compute_domain[N - 2]),
        static_cast<size_t>(compute_domain[N - 3])};
    const pfe_wrapper<N, Kernel> _pf(compute_domain, f);
    return completion_future(std::shared_future<void>(mcw_cxxamp_launch_kernel_async<pfe_wrapper<N, Kernel>, 3>(ext, NULL, _pf)));
#else
    auto bar = &pfe_wrapper<N, Kernel>::operator();
    auto qq = &index<N>::__cxxamp_opencl_index;
    int* foo = reinterpret_cast<int*>(&pfe_wrapper<N, Kernel>::__cxxamp_trampoline);
#endif
}
#pragma clang diagnostic pop

template class index<1>;
//1D parallel_for_each, nontiled
template <typename Kernel>
__attribute__((noinline,used)) void parallel_for_each(
    extent<1> compute_domain,
    const Kernel& f) restrict(cpu,amp) {
#ifndef __GPU__
  if(compute_domain[0]<=0) {
    throw invalid_compute_domain("Extent is less or equal than 0.");
  }
  if (static_cast<size_t>(compute_domain[0]) > 4294967295L) 
    throw invalid_compute_domain("Extent size too large.");
  size_t ext = compute_domain[0];
  accelerator_view accl_view = accelerator::get_auto_selection_view();
  mcw_cxxamp_launch_kernel<Kernel, 1>(&ext, NULL, f, accl_view);
#else //ifndef __GPU__
  //to ensure functor has right operator() defined
  //this triggers the trampoline code being emitted
  int* foo = reinterpret_cast<int*>(&Kernel::__cxxamp_trampoline);
#endif
}

//1D parallel_for_each, nontiled, with accelerator_view
template <typename Kernel>
__attribute__((noinline,used)) void parallel_for_each(
    const accelerator_view& accl_view,
    extent<1> compute_domain,
    const Kernel& f) restrict(cpu,amp) {
#ifndef __GPU__
  if(compute_domain[0]<=0) {
    throw invalid_compute_domain("Extent is less or equal than 0.");
  }
  if (static_cast<size_t>(compute_domain[0]) > 4294967295L) 
    throw invalid_compute_domain("Extent size too large.");
  size_t ext = compute_domain[0];
  mcw_cxxamp_launch_kernel<Kernel, 1>(&ext, NULL, f, accl_view);
#else //ifndef __GPU__
  //to ensure functor has right operator() defined
  //this triggers the trampoline code being emitted
  int* foo = reinterpret_cast<int*>(&Kernel::__cxxamp_trampoline);
#endif
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"
//1D async_parallel_for_each, nontiled
template <typename Kernel>
__attribute__((noinline,used)) completion_future async_parallel_for_each(
    extent<1> compute_domain,
    const Kernel& f) restrict(cpu,amp) {
#ifndef __GPU__
  if(compute_domain[0]<=0) {
    throw invalid_compute_domain("Extent is less or equal than 0.");
  }
  if (static_cast<size_t>(compute_domain[0]) > 4294967295L) 
    throw invalid_compute_domain("Extent size too large.");
  size_t ext = compute_domain[0];
  return completion_future(std::shared_future<void>(mcw_cxxamp_launch_kernel_async<Kernel, 1>(&ext, NULL, f)));
#else //ifndef __GPU__
  //to ensure functor has right operator() defined
  //this triggers the trampoline code being emitted
  int* foo = reinterpret_cast<int*>(&Kernel::__cxxamp_trampoline);
#endif
}
#pragma clang diagnostic pop

template class index<2>;
//2D parallel_for_each, nontiled
template <typename Kernel>
__attribute__((noinline,used)) void parallel_for_each(
    extent<2> compute_domain,
    const Kernel& f) restrict(cpu,amp) {
#ifndef __GPU__
  if(compute_domain[0]<=0 || compute_domain[1]<=0) {
    throw invalid_compute_domain("Extent is less or equal than 0.");
  }
  if (static_cast<size_t>(compute_domain[0]) * static_cast<size_t>(compute_domain[1]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  size_t ext[2] = {static_cast<size_t>(compute_domain[1]),
                   static_cast<size_t>(compute_domain[0])};
  accelerator_view accl_view = accelerator::get_auto_selection_view();
  mcw_cxxamp_launch_kernel<Kernel, 2>(ext, NULL, f, accl_view);
#else //ifndef __GPU__
  //to ensure functor has right operator() defined
  //this triggers the trampoline code being emitted
  int* foo = reinterpret_cast<int*>(&Kernel::__cxxamp_trampoline);
#endif
}

//2D parallel_for_each, nontiled, with accelerator_view
template <typename Kernel>
__attribute__((noinline,used)) void parallel_for_each(
    const accelerator_view& accl_view,
    extent<2> compute_domain,
    const Kernel& f) restrict(cpu,amp) {
#ifndef __GPU__
  if(compute_domain[0]<=0 || compute_domain[1]<=0) {
    throw invalid_compute_domain("Extent is less or equal than 0.");
  }
  if (static_cast<size_t>(compute_domain[0]) * static_cast<size_t>(compute_domain[1]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  size_t ext[2] = {static_cast<size_t>(compute_domain[1]),
                   static_cast<size_t>(compute_domain[0])};
  mcw_cxxamp_launch_kernel<Kernel, 2>(ext, NULL, f, accl_view);
#else //ifndef __GPU__
  //to ensure functor has right operator() defined
  //this triggers the trampoline code being emitted
  int* foo = reinterpret_cast<int*>(&Kernel::__cxxamp_trampoline);
#endif
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"
//2D async_parallel_for_each, nontiled
template <typename Kernel>
__attribute__((noinline,used)) completion_future async_parallel_for_each(
    extent<2> compute_domain,
    const Kernel& f) restrict(cpu,amp) {
#ifndef __GPU__
  if(compute_domain[0]<=0 || compute_domain[1]<=0) {
    throw invalid_compute_domain("Extent is less or equal than 0.");
  }
  if (static_cast<size_t>(compute_domain[0]) * static_cast<size_t>(compute_domain[1]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  size_t ext[2] = {static_cast<size_t>(compute_domain[1]),
                   static_cast<size_t>(compute_domain[0])};
  return completion_future(std::shared_future<void>(mcw_cxxamp_launch_kernel_async<Kernel, 2>(ext, NULL, f)));
#else //ifndef __GPU__
  //to ensure functor has right operator() defined
  //this triggers the trampoline code being emitted
  int* foo = reinterpret_cast<int*>(&Kernel::__cxxamp_trampoline);
#endif
}
#pragma clang diagnostic pop

template class index<3>;
//3D parallel_for_each, nontiled
template <typename Kernel>
__attribute__((noinline,used)) void parallel_for_each(
    extent<3> compute_domain,
    const Kernel& f) restrict(cpu,amp) {
#ifndef __GPU__
  if(compute_domain[0]<=0 || compute_domain[1]<=0 || compute_domain[2]<=0) {
    throw invalid_compute_domain("Extent is less or equal than 0.");
  }
  if (static_cast<size_t>(compute_domain[0]) * static_cast<size_t>(compute_domain[1]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  if (static_cast<size_t>(compute_domain[1]) * static_cast<size_t>(compute_domain[2]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  if (static_cast<size_t>(compute_domain[0]) * static_cast<size_t>(compute_domain[2]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  if (static_cast<size_t>(compute_domain[0]) * static_cast<size_t>(compute_domain[1]) * static_cast<size_t>(compute_domain[2]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  size_t ext[3] = {static_cast<size_t>(compute_domain[2]),
                   static_cast<size_t>(compute_domain[1]),
                   static_cast<size_t>(compute_domain[0])};
  accelerator_view accl_view = accelerator::get_auto_selection_view();
  mcw_cxxamp_launch_kernel<Kernel, 3>(ext, NULL, f, accl_view);
#else //ifndef __GPU__
  //to ensure functor has right operator() defined
  //this triggers the trampoline code being emitted
  int* foo = reinterpret_cast<int*>(&Kernel::__cxxamp_trampoline);
#endif
}

//3D parallel_for_each, nontiled, with accelerator_view
template <typename Kernel>
__attribute__((noinline,used)) void parallel_for_each(
    const accelerator_view& accl_view,
    extent<3> compute_domain,
    const Kernel& f) restrict(cpu,amp) {
#ifndef __GPU__
  if(compute_domain[0]<=0 || compute_domain[1]<=0 || compute_domain[2]<=0) {
    throw invalid_compute_domain("Extent is less or equal than 0.");
  }
  if (static_cast<size_t>(compute_domain[0]) * static_cast<size_t>(compute_domain[1]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  if (static_cast<size_t>(compute_domain[1]) * static_cast<size_t>(compute_domain[2]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  if (static_cast<size_t>(compute_domain[0]) * static_cast<size_t>(compute_domain[2]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  if (static_cast<size_t>(compute_domain[0]) * static_cast<size_t>(compute_domain[1]) * static_cast<size_t>(compute_domain[2]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  size_t ext[3] = {static_cast<size_t>(compute_domain[2]),
                   static_cast<size_t>(compute_domain[1]),
                   static_cast<size_t>(compute_domain[0])};
  mcw_cxxamp_launch_kernel<Kernel, 3>(ext, NULL, f, accl_view);
#else //ifndef __GPU__
  //to ensure functor has right operator() defined
  //this triggers the trampoline code being emitted
  int* foo = reinterpret_cast<int*>(&Kernel::__cxxamp_trampoline);
#endif
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"
//3D async_parallel_for_each, nontiled
template <typename Kernel>
__attribute__((noinline,used)) completion_future async_parallel_for_each(
    extent<3> compute_domain,
    const Kernel& f) restrict(cpu,amp) {
#ifndef __GPU__
  if(compute_domain[0]<=0 || compute_domain[1]<=0 || compute_domain[2]<=0) {
    throw invalid_compute_domain("Extent is less or equal than 0.");
  }
  if (static_cast<size_t>(compute_domain[0]) * static_cast<size_t>(compute_domain[1]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  if (static_cast<size_t>(compute_domain[1]) * static_cast<size_t>(compute_domain[2]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  if (static_cast<size_t>(compute_domain[0]) * static_cast<size_t>(compute_domain[2]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  if (static_cast<size_t>(compute_domain[0]) * static_cast<size_t>(compute_domain[1]) * static_cast<size_t>(compute_domain[2]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  size_t ext[3] = {static_cast<size_t>(compute_domain[2]),
                   static_cast<size_t>(compute_domain[1]),
                   static_cast<size_t>(compute_domain[0])};
  return completion_future(std::shared_future<void>(mcw_cxxamp_launch_kernel_async<Kernel, 3>(ext, NULL, f)));
#else //ifndef __GPU__
  //to ensure functor has right operator() defined
  //this triggers the trampoline code being emitted
  int* foo = reinterpret_cast<int*>(&Kernel::__cxxamp_trampoline);
#endif
}
#pragma clang diagnostic pop

//1D parallel_for_each, tiled
template <int D0, typename Kernel>
__attribute__((noinline,used)) void parallel_for_each(
    tiled_extent<D0> compute_domain,
    const Kernel& f) restrict(cpu,amp) {
#ifndef __GPU__
  if(compute_domain[0]<=0) {
    throw invalid_compute_domain("Extent is less or equal than 0.");
  }
  if (static_cast<size_t>(compute_domain[0]) > 4294967295L) 
    throw invalid_compute_domain("Extent size too large.");
  size_t ext = compute_domain[0];
  size_t tile = compute_domain.tile_dim0;
  static_assert( compute_domain.tile_dim0 <= 1024, "The maximum nuimber of threads in a tile is 1024");
  if(ext % tile != 0) {
    throw invalid_compute_domain("Extent can't be evenly divisble by tile size.");
  }
  accelerator_view accl_view = accelerator::get_auto_selection_view();
  mcw_cxxamp_launch_kernel<Kernel, 1>(&ext, &tile, f, accl_view);
#else //ifndef __GPU__
  tiled_index<D0> this_is_used_to_instantiate_the_right_index;
  //to ensure functor has right operator() defined
  //this triggers the trampoline code being emitted
  int* foo = reinterpret_cast<int*>(&Kernel::__cxxamp_trampoline);
#endif
}
//1D parallel_for_each, tiled, with accelerator_view
template <int D0, typename Kernel>
__attribute__((noinline,used)) void parallel_for_each(
    const accelerator_view& accl_view,
    tiled_extent<D0> compute_domain,
    const Kernel& f) restrict(cpu,amp) {
#ifndef __GPU__
  if(compute_domain[0]<=0) {
    throw invalid_compute_domain("Extent is less or equal than 0.");
  }
  if (static_cast<size_t>(compute_domain[0]) > 4294967295L) 
    throw invalid_compute_domain("Extent size too large.");
  size_t ext = compute_domain[0];
  size_t tile = compute_domain.tile_dim0;
  static_assert( compute_domain.tile_dim0 <= 1024, "The maximum nuimber of threads in a tile is 1024");
  if(ext % tile != 0) {
    throw invalid_compute_domain("Extent can't be evenly divisble by tile size.");
  }
  mcw_cxxamp_launch_kernel<Kernel, 1>(&ext, &tile, f, accl_view);
#else //ifndef __GPU__
  tiled_index<D0> this_is_used_to_instantiate_the_right_index;
  //to ensure functor has right operator() defined
  //this triggers the trampoline code being emitted
  int* foo = reinterpret_cast<int*>(&Kernel::__cxxamp_trampoline);
#endif
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"
//1D async_parallel_for_each, tiled
template <int D0, typename Kernel>
__attribute__((noinline,used)) completion_future async_parallel_for_each(
    tiled_extent<D0> compute_domain,
    const Kernel& f) restrict(cpu,amp) {
#ifndef __GPU__
  if(compute_domain[0]<=0) {
    throw invalid_compute_domain("Extent is less or equal than 0.");
  }
  if (static_cast<size_t>(compute_domain[0]) > 4294967295L) 
    throw invalid_compute_domain("Extent size too large.");
  size_t ext = compute_domain[0];
  size_t tile = compute_domain.tile_dim0;
  static_assert( compute_domain.tile_dim0 <= 1024, "The maximum nuimber of threads in a tile is 1024");
  if(ext % tile != 0) {
    throw invalid_compute_domain("Extent can't be evenly divisble by tile size.");
  }
  return completion_future(std::shared_future<void>(mcw_cxxamp_launch_kernel_async<Kernel, 1>(&ext, &tile, f)));
#else //ifndef __GPU__
  tiled_index<D0> this_is_used_to_instantiate_the_right_index;
  //to ensure functor has right operator() defined
  //this triggers the trampoline code being emitted
  int* foo = reinterpret_cast<int*>(&Kernel::__cxxamp_trampoline);
#endif
}
#pragma clang diagnostic pop

//2D parallel_for_each, tiled
template <int D0, int D1, typename Kernel>
__attribute__((noinline,used)) void parallel_for_each(
    tiled_extent<D0, D1> compute_domain,
    const Kernel& f) restrict(cpu,amp) {
#ifndef __GPU__
  if(compute_domain[0]<=0 || compute_domain[1]<=0) {
    throw invalid_compute_domain("Extent is less or equal than 0.");
  }
  if (static_cast<size_t>(compute_domain[0]) * static_cast<size_t>(compute_domain[1]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  size_t ext[2] = { static_cast<size_t>(compute_domain[1]),
                    static_cast<size_t>(compute_domain[0])};
  size_t tile[2] = { compute_domain.tile_dim1,
                     compute_domain.tile_dim0};
  static_assert( (compute_domain.tile_dim1 * compute_domain.tile_dim0)<= 1024, "The maximum nuimber of threads in a tile is 1024");
  if((ext[0] % tile[0] != 0) || (ext[1] % tile[1] != 0)) {
    throw invalid_compute_domain("Extent can't be evenly divisble by tile size.");
  }
  accelerator_view accl_view = accelerator::get_auto_selection_view();
  mcw_cxxamp_launch_kernel<Kernel, 2>(ext, tile, f, accl_view);
#else //ifndef __GPU__
  tiled_index<D0, D1> this_is_used_to_instantiate_the_right_index;
  //to ensure functor has right operator() defined
  //this triggers the trampoline code being emitted
  int* foo = reinterpret_cast<int*>(&Kernel::__cxxamp_trampoline);
#endif
}

//2D parallel_for_each, tiled, with accelerator_view
template <int D0, int D1, typename Kernel>
__attribute__((noinline,used)) void parallel_for_each(
    const accelerator_view& accl_view,
    tiled_extent<D0, D1> compute_domain,
    const Kernel& f) restrict(cpu,amp) {
#ifndef __GPU__
  if(compute_domain[0]<=0 || compute_domain[1]<=0) {
    throw invalid_compute_domain("Extent is less or equal than 0.");
  }
  if (static_cast<size_t>(compute_domain[0]) * static_cast<size_t>(compute_domain[1]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  size_t ext[2] = { static_cast<size_t>(compute_domain[1]),
                    static_cast<size_t>(compute_domain[0])};
  size_t tile[2] = { compute_domain.tile_dim1,
                     compute_domain.tile_dim0};
  static_assert( (compute_domain.tile_dim1 * compute_domain.tile_dim0)<= 1024, "The maximum nuimber of threads in a tile is 1024");
  if((ext[0] % tile[0] != 0) || (ext[1] % tile[1] != 0)) {
    throw invalid_compute_domain("Extent can't be evenly divisble by tile size.");
  }
  mcw_cxxamp_launch_kernel<Kernel, 2>(ext, tile, f, accl_view);
#else //ifndef __GPU__
  tiled_index<D0, D1> this_is_used_to_instantiate_the_right_index;
  //to ensure functor has right operator() defined
  //this triggers the trampoline code being emitted
  int* foo = reinterpret_cast<int*>(&Kernel::__cxxamp_trampoline);
#endif
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"
//2D async_parallel_for_each, tiled
template <int D0, int D1, typename Kernel>
__attribute__((noinline,used)) completion_future async_parallel_for_each(
    tiled_extent<D0, D1> compute_domain,
    const Kernel& f) restrict(cpu,amp) {
#ifndef __GPU__
  if(compute_domain[0]<=0 || compute_domain[1]<=0) {
    throw invalid_compute_domain("Extent is less or equal than 0.");
  }
  if (static_cast<size_t>(compute_domain[0]) * static_cast<size_t>(compute_domain[1]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  size_t ext[2] = { static_cast<size_t>(compute_domain[1]),
                    static_cast<size_t>(compute_domain[0])};
  size_t tile[2] = { compute_domain.tile_dim1,
                     compute_domain.tile_dim0};
  static_assert( (compute_domain.tile_dim1 * compute_domain.tile_dim0)<= 1024, "The maximum nuimber of threads in a tile is 1024");
  if((ext[0] % tile[0] != 0) || (ext[1] % tile[1] != 0)) {
    throw invalid_compute_domain("Extent can't be evenly divisble by tile size.");
  }
  return completion_future(std::shared_future<void>(mcw_cxxamp_launch_kernel_async<Kernel, 2>(ext, tile, f)));
#else //ifndef __GPU__
  tiled_index<D0, D1> this_is_used_to_instantiate_the_right_index;
  //to ensure functor has right operator() defined
  //this triggers the trampoline code being emitted
  int* foo = reinterpret_cast<int*>(&Kernel::__cxxamp_trampoline);
#endif
}
#pragma clang diagnostic pop

//3D parallel_for_each, tiled
template <int D0, int D1, int D2, typename Kernel>
__attribute__((noinline,used)) void parallel_for_each(
    tiled_extent<D0, D1, D2> compute_domain,
    const Kernel& f) restrict(cpu,amp) {
#ifndef __GPU__
  if(compute_domain[0]<=0 || compute_domain[1]<=0 || compute_domain[2]<=0) {
    throw invalid_compute_domain("Extent is less or equal than 0.");
  }
  if (static_cast<size_t>(compute_domain[0]) * static_cast<size_t>(compute_domain[1]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  if (static_cast<size_t>(compute_domain[1]) * static_cast<size_t>(compute_domain[2]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  if (static_cast<size_t>(compute_domain[0]) * static_cast<size_t>(compute_domain[2]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  if (static_cast<size_t>(compute_domain[0]) * static_cast<size_t>(compute_domain[1]) * static_cast<size_t>(compute_domain[2]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  size_t ext[3] = { static_cast<size_t>(compute_domain[2]),
                    static_cast<size_t>(compute_domain[1]),
                    static_cast<size_t>(compute_domain[0])};
  size_t tile[3] = { compute_domain.tile_dim2,
                     compute_domain.tile_dim1,
                     compute_domain.tile_dim0};
  static_assert(( compute_domain.tile_dim2 * compute_domain.tile_dim1* compute_domain.tile_dim0)<= 1024, "The maximum nuimber of threads in a tile is 1024");
  if((ext[0] % tile[0] != 0) || (ext[1] % tile[1] != 0) || (ext[2] % tile[2] != 0)) {
    throw invalid_compute_domain("Extent can't be evenly divisble by tile size.");
  }
  accelerator_view accl_view = accelerator::get_auto_selection_view();
  mcw_cxxamp_launch_kernel<Kernel, 3>(ext, tile, f, accl_view);
#else //ifndef __GPU__
  tiled_index<D0, D1, D2> this_is_used_to_instantiate_the_right_index;
  //to ensure functor has right operator() defined
  //this triggers the trampoline code being emitted
  int* foo = reinterpret_cast<int*>(&Kernel::__cxxamp_trampoline);
#endif
}

//3D parallel_for_each, tiled, with accelerator_view
template <int D0, int D1, int D2, typename Kernel>
__attribute__((noinline,used)) void parallel_for_each(
    const accelerator_view& accl_view,
    tiled_extent<D0, D1, D2> compute_domain,
    const Kernel& f) restrict(cpu,amp) {
#ifndef __GPU__
  if(compute_domain[0]<=0 || compute_domain[1]<=0 || compute_domain[2]<=0) {
    throw invalid_compute_domain("Extent is less or equal than 0.");
  }
  if (static_cast<size_t>(compute_domain[0]) * static_cast<size_t>(compute_domain[1]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  if (static_cast<size_t>(compute_domain[1]) * static_cast<size_t>(compute_domain[2]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  if (static_cast<size_t>(compute_domain[0]) * static_cast<size_t>(compute_domain[2]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  if (static_cast<size_t>(compute_domain[0]) * static_cast<size_t>(compute_domain[1]) * static_cast<size_t>(compute_domain[2]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  size_t ext[3] = { static_cast<size_t>(compute_domain[2]),
                    static_cast<size_t>(compute_domain[1]),
                    static_cast<size_t>(compute_domain[0])};
  size_t tile[3] = { compute_domain.tile_dim2,
                     compute_domain.tile_dim1,
                     compute_domain.tile_dim0};
  static_assert(( compute_domain.tile_dim2 * compute_domain.tile_dim1* compute_domain.tile_dim0)<= 1024, "The maximum nuimber of threads in a tile is 1024");
  if((ext[0] % tile[0] != 0) || (ext[1] % tile[1] != 0) || (ext[2] % tile[2] != 0)) {
    throw invalid_compute_domain("Extent can't be evenly divisble by tile size.");
  }
  mcw_cxxamp_launch_kernel<Kernel, 3>(ext, tile, f, accl_view);
#else //ifndef __GPU__
  tiled_index<D0, D1, D2> this_is_used_to_instantiate_the_right_index;
  //to ensure functor has right operator() defined
  //this triggers the trampoline code being emitted
  int* foo = reinterpret_cast<int*>(&Kernel::__cxxamp_trampoline);
#endif
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"
//3D async_parallel_for_each, tiled
template <int D0, int D1, int D2, typename Kernel>
__attribute__((noinline,used)) completion_future async_parallel_for_each(
    tiled_extent<D0, D1, D2> compute_domain,
    const Kernel& f) restrict(cpu,amp) {
#ifndef __GPU__
  if(compute_domain[0]<=0 || compute_domain[1]<=0 || compute_domain[2]<=0) {
    throw invalid_compute_domain("Extent is less or equal than 0.");
  }
  if (static_cast<size_t>(compute_domain[0]) * static_cast<size_t>(compute_domain[1]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  if (static_cast<size_t>(compute_domain[1]) * static_cast<size_t>(compute_domain[2]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  if (static_cast<size_t>(compute_domain[0]) * static_cast<size_t>(compute_domain[2]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  if (static_cast<size_t>(compute_domain[0]) * static_cast<size_t>(compute_domain[1]) * static_cast<size_t>(compute_domain[2]) > 4294967295L)
    throw invalid_compute_domain("Extent size too large.");
  size_t ext[3] = { static_cast<size_t>(compute_domain[2]),
                    static_cast<size_t>(compute_domain[1]),
                    static_cast<size_t>(compute_domain[0])};
  size_t tile[3] = { compute_domain.tile_dim2,
                     compute_domain.tile_dim1,
                     compute_domain.tile_dim0};
  static_assert(( compute_domain.tile_dim2 * compute_domain.tile_dim1* compute_domain.tile_dim0)<= 1024, "The maximum nuimber of threads in a tile is 1024");
  if((ext[0] % tile[0] != 0) || (ext[1] % tile[1] != 0) || (ext[2] % tile[2] != 0)) {
    throw invalid_compute_domain("Extent can't be evenly divisble by tile size.");
  }
  return completion_future(std::shared_future<void>(mcw_cxxamp_launch_kernel_async<Kernel, 3>(ext, tile, f)));
#else //ifndef __GPU__
  tiled_index<D0, D1, D2> this_is_used_to_instantiate_the_right_index;
  //to ensure functor has right operator() defined
  //this triggers the trampoline code being emitted
  int* foo = reinterpret_cast<int*>(&Kernel::__cxxamp_trampoline);
#endif
}
#pragma clang diagnostic pop

} // namespace Concurrency
