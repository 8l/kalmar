//===----------------------------------------------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// FIXME this file will place C++AMP Runtime implementation (OpenCL version)

#include <amp.h>
#include <CL/opencl.h>
namespace Concurrency {

AMPAllocator& getAllocator()
{
    static AMPAllocator amp;
    return amp;
}

} // namespace Concurrency

namespace {
bool __mcw_cxxamp_compiled = false;
}

#ifdef __APPLE__
#include <mach-o/getsect.h>
extern "C" intptr_t _dyld_get_image_vmaddr_slide(uint32_t image_index);
#else
extern "C" char * kernel_source_[] asm ("_binary_kernel_cl_start") __attribute__((weak));
extern "C" char * kernel_size_[] asm ("_binary_kernel_cl_size") __attribute__((weak));
#endif

extern std::vector<std::string> __mcw_kernel_names;

namespace Concurrency {
namespace CLAMP {

void *CreateKernel(std::string s) {
  cl_int err;
  AMPAllocator& aloc = getAllocator();
  aloc.compile();
  aloc.kernel = clCreateKernel(aloc.program, s.c_str(), &err);
  assert(err == CL_SUCCESS);
  return aloc.kernel;
}

std::future<void> LaunchKernelAsync(void *ker, size_t nr_dim, size_t *global, size_t *local) {
  throw runtime_exception("async_parallel_for_each is unsupported on this platform", 0);
}

void LaunchKernel(void *kernel, size_t dim_ext, size_t *ext, size_t *local_size) {
  cl_int err;
  AMPAllocator& aloc = getAllocator();
  {
      // C++ AMP specifications
      // The maximum number of tiles per dimension will be no less than 65535.
      // The maximum number of threads in a tile will be no less than 1024.
      // In 3D tiling, the maximal value of D0 will be no less than 64.
      cl_uint dimensions;
      err = clGetDeviceInfo(aloc.device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(cl_uint), &dimensions, NULL);
      size_t *maxSizes = new size_t[dimensions];
      err = clGetDeviceInfo(aloc.device, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t) * dimensions, maxSizes, NULL);
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
  aloc.write();
#endif
  err = clEnqueueNDRangeKernel(aloc.queue, aloc.kernel, dim_ext, NULL, ext, local_size, 0, NULL, NULL);
  assert(err == CL_SUCCESS);
#if defined(CXXAMP_NV)
  aloc.read();
#endif
  clFinish(aloc.queue);
}


    static inline void getKernelNames(cl_program& prog) {
        std::vector<std::string> n;
        cl_uint kernel_num = 0;
        cl_uint ret = CL_SUCCESS;
        char **names;
        int count;
        ret = clCreateKernelsInProgram(prog, 1024, NULL, &kernel_num);
        if (ret == CL_SUCCESS && kernel_num > 0) {
            cl_kernel *kl = new cl_kernel[kernel_num];
            ret = clCreateKernelsInProgram(prog, kernel_num + 1, kl, &kernel_num);
            if (ret == CL_SUCCESS) {
                std::map<std::string, std::string> aMap;
                for (unsigned i = 0; i < unsigned(kernel_num); ++i) {
                    char s[1024] = { 0x0 };
                    size_t size;
                    ret = clGetKernelInfo(kl[i], CL_KERNEL_FUNCTION_NAME, 1024, s, &size);
                    n.push_back(std::string (s));
                    clReleaseKernel(kl[i]);
                }
            }
            delete [] kl;
        }
        if (n.size()) {
            std::sort(n.begin(), n.end());
            n.erase(std::unique(n.begin(), n.end()), n.end());
        }
        if (n.size()) {
            names = new char *[n.size()];
            int i = 0;
            std::vector<std::string>::iterator it;
            for (it = n.begin(); it != n.end(); ++it, ++i) {
                size_t len = (*it).length();
                char *name = new char[len + 1];
                memcpy(name, (*it).c_str(), len);
                name[len] = '\0';
                names[i] = name;
            }
            count = unsigned(n.size());
        }
        if (count) {
            int i = 0;
            while (names && i < count) {
                __mcw_kernel_names.push_back(std::string(names[i]));
                delete [] names[i];
                ++i;
            }
            delete [] names;
            if (__mcw_kernel_names.size()) {
                std::sort(std::begin(__mcw_kernel_names), std::end(__mcw_kernel_names));
                __mcw_kernel_names.erase (std::unique (__mcw_kernel_names.begin (),
                                                       __mcw_kernel_names.end ()),
                                          __mcw_kernel_names.end ());
            }
        }
    }

    void CompileKernels(cl_program& program, cl_context& context, cl_device_id& device)
    {
        cl_int err;
        if (!__mcw_cxxamp_compiled) {
#ifdef __APPLE__
            const struct section_64 *sect = getsectbyname("binary", "kernel_cl");
            unsigned char *kernel_source = (unsigned char*)calloc(1, sect->size+1);
            size_t kernel_size = sect->size;
            assert(sect->addr != 0);
            memcpy(kernel_source, (void*)(sect->addr + _dyld_get_image_vmaddr_slide(0)), kernel_size); // whatever
#else
            size_t kernel_size = (size_t)((void *)kernel_size_);
            unsigned char *kernel_source = (unsigned char*)malloc(kernel_size+1);
            memcpy(kernel_source, kernel_source_, kernel_size);
#endif
            kernel_source[kernel_size] = '\0';
            if (kernel_source[0] == 'B' && kernel_source[1] == 'C') {
                // Bitcode magic number. Assuming it's in SPIR
                const unsigned char *ks = (const unsigned char *)kernel_source;
                program = clCreateProgramWithBinary(context, 1, &device, &kernel_size, &ks, NULL, &err);
                if (err == CL_SUCCESS)
                    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
            } else {
                // in OpenCL-C
                const char *ks = (const char *)kernel_source;
                program = clCreateProgramWithSource(context, 1, &ks, &kernel_size, &err);
                if (err == CL_SUCCESS)
                    err = clBuildProgram(program, 1, &device, "-D__ATTRIBUTE_WEAK__=", NULL, NULL);
            }
            if (err != CL_SUCCESS) {
                size_t len;
                err = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &len);
                assert(err == CL_SUCCESS);
                char *msg = new char[len + 1];
                err = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, len, msg, NULL);
                assert(err == CL_SUCCESS);
                msg[len] = '\0';
                std::cerr << msg;
                delete [] msg;
                exit(1);
            }
            __mcw_cxxamp_compiled = true;
            free(kernel_source);
            getKernelNames(program);
        }
    }

} // namespce CLAMP
} // namespace Concurrency