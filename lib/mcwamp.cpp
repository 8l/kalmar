//===----------------------------------------------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <amp.h>
#include <map>
#include <CL/opencl.h>
#include <amprt.h>

#include <md5.h>
#include <sstream>
#include <fstream>
#include <iomanip>

namespace Concurrency {

// initialize static class members
const wchar_t accelerator::gpu_accelerator[] = L"gpu";
const wchar_t accelerator::cpu_accelerator[] = L"cpu";
const wchar_t accelerator::default_accelerator[] = L"default";
#if defined(CXXAMP_ENABLE_HSA)
std::shared_ptr<accelerator> accelerator::_gpu_accelerator = std::make_shared<accelerator>(accelerator::gpu_accelerator);
#endif
std::shared_ptr<accelerator> accelerator::_cpu_accelerator = std::make_shared<accelerator>(accelerator::cpu_accelerator);
std::shared_ptr<accelerator> accelerator::_default_accelerator = std::make_shared<accelerator>(L"gpu0");

std::vector<accelerator> accelerator::_accs;
cl_device_id details::DeviceManager::starting_id = NULL;
std::map<cl_device_id, struct DimMaxSize> Clid2DimSizeMap;
static details::DeviceManager DeviceMgr;
AMPAllocator* getAllocator(cl_device_id id)
{
  return DeviceMgr.getAllocator(id);
}
namespace CLAMP {
cl_device_id getAvailableAccelerator()
{
  return DeviceMgr.getAvailableDevice();
}
}
void* getDevicePointer(void* data) {
  return DeviceMgr.getDevicePointer(data);
}
cl_command_queue getOCLQueue(void* device_ptr) {
  return DeviceMgr.getOCLQueue(device_ptr);
}
std::map<cl_kernel, std::vector<cl_event> > kernelEventMap;
//int mm_info::waitOnKernelsCount = 0;
} // namespace Concurrency

namespace {
std::map<cl_device_id, bool>__mcw_cxxamp_compiled;
}

#ifdef __APPLE__
#include <mach-o/getsect.h>
extern "C" intptr_t _dyld_get_image_vmaddr_slide(uint32_t image_index);
#else
extern "C" char * kernel_source_[] asm ("_binary_kernel_cl_start");
extern "C" char * kernel_source_end_[] asm ("_binary_kernel_cl_end");
#endif

std::vector<std::string> __mcw_kernel_names;
namespace Concurrency {
namespace CLAMP {
typedef std::map<std::string, cl_kernel> KernelObject;
#if CACHE_KERNEL_OBJECT
std::map<cl_program, KernelObject> Pro2KernelObject;
#endif
    static inline void getKernelNames(cl_program& prog) {
        std::vector<std::string> n;
        cl_uint kernel_num = 0;
        cl_uint ret = CL_SUCCESS;
        char **names;
        int count = 0;
        ret = clCreateKernelsInProgram(prog, 1024, NULL, &kernel_num);
        if (ret == CL_SUCCESS && kernel_num > 0) {
            cl_kernel *kl = new cl_kernel[kernel_num];
            ret = clCreateKernelsInProgram(prog, kernel_num + 1, kl, &kernel_num);
            if (ret == CL_SUCCESS) {
                #if CACHE_KERNEL_OBJECT
                KernelObject& KO = Pro2KernelObject[prog];
                #endif
                std::map<std::string, std::string> aMap;
                for (unsigned i = 0; i < unsigned(kernel_num); ++i) {
                    char s[1024] = { 0x0 };
                    size_t size;
                    ret = clGetKernelInfo(kl[i], CL_KERNEL_FUNCTION_NAME, 1024, s, &size);
                    n.push_back(std::string (s));
                    #if CACHE_KERNEL_OBJECT
                    KO[std::string (s)] = kl[i];
                    #else
                    clReleaseKernel(kl[i]);
                    #endif
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
// Levenshtein Distance to measure the difference of two sequences
// The shortest distance it returns the more likely the two sequences are equal
static inline int ldistance(const std::string source, const std::string target)
{
  int n = source.length();
  int m = target.length();
  if (m == 0)
    return n;
  if (n == 0)
    return m;
  
  //Construct a matrix
  typedef std::vector < std::vector < int >>Tmatrix;
  Tmatrix matrix(n + 1);
  
  for (int i = 0; i <= n; i++)
    matrix[i].resize(m + 1);
  for (int i = 1; i <= n; i++)
    matrix[i][0] = i;
  for (int i = 1; i <= m; i++)
    matrix[0][i] = i;

  for (int i = 1; i <= n; i++) {
    const char si = source[i - 1];
    for (int j = 1; j <= m; j++) {
      const char dj = target[j - 1];
      int cost;
      if (si == dj)
        cost = 0;
      else
        cost = 1;
      const int above = matrix[i - 1][j] + 1;
      const int left = matrix[i][j - 1] + 1;
      const int diag = matrix[i - 1][j - 1] + cost;
      matrix[i][j] = std::min(above, std::min(left, diag));
    }
  }
  return matrix[n][m];
}
// transformed_kernel_name (mangled) might differ if usages of 'm32' flag in CPU/GPU
// paths are mutually exclusive. We can scan all kernel names and replace
// transformed_kernel_name with the one that has the shortest distance from it by using 
// Levenshtein Distance measurement
void MatchKernelNames(std::string& fixed_name) {
  if (__mcw_kernel_names.size()) {
    // Must start from a big value > 10
    int distance = 1024;
    int hit = -1;
    std::string shortest;
    for (std::vector < std::string >::iterator it = __mcw_kernel_names.begin();
         it != __mcw_kernel_names.end(); ++it) {
      if ((*it) == fixed_name) {
        // Perfect match. Mark no need to replace and skip the loop
        hit = -1;
        break;
      }
      int n = ldistance(fixed_name, (*it));
      if (n <= distance) {
        distance = n;
        hit = 1;
        shortest = (*it);
      }
    }
    /* Replacement. Skip if not hit or the distance is too far (>5)*/
    if (hit >= 0 && distance < 5)
      fixed_name = shortest;
  }
  return;
}
cl_kernel GetKernelObject(cl_program& prog, std::string& name) {
  #if !CACHE_KERNEL_OBJECT
  assert (name.c_str());
  cl_int err;
  cl_kernel kernel;
  kernel = clCreateKernel(prog, name.c_str(), &err);
  assert(err == CL_SUCCESS);
  return kernel;
  #else
  KernelObject& KO = Pro2KernelObject[prog];
  if (KO[name] == 0) {
    cl_int err;
    KO[name] = clCreateKernel(prog, name.c_str(), &err);
    assert(err == CL_SUCCESS);
  }
  return KO[name];
  #endif
}
#if CACHE_KERNEL_OBJECT
void ReleaseKernelObject() {
  for(const auto& it : Pro2KernelObject)
    for(const auto& itt : it.second) 
      if(itt.second)
        clReleaseKernel(itt.second);
}
#endif
}
}
namespace Concurrency { namespace CLAMP {
    void CompileKernels(cl_program& program, cl_context& context, cl_device_id& device)
    {
#ifdef CXXAMP_ENABLE_HSA
        assert(0 && "Unsupported function");
#else
        cl_int err;
        if (!__mcw_cxxamp_compiled[device]) {
#ifdef __APPLE__
            const struct section_64 *sect = getsectbyname("binary", "kernel_cl");
            unsigned char *kernel_source = (unsigned char*)calloc(1, sect->size+1);
            size_t kernel_size = sect->size;
            assert(sect->addr != 0);
            memcpy(kernel_source, (void*)(sect->addr + _dyld_get_image_vmaddr_slide(0)), kernel_size); // whatever
#else
            size_t kernel_size =
                (ptrdiff_t)((void *)kernel_source_end_) -
                (ptrdiff_t)((void *)kernel_source_);
            unsigned char *kernel_source = (unsigned char*)malloc(kernel_size+1);
            memcpy(kernel_source, kernel_source_, kernel_size);
#endif
            kernel_source[kernel_size] = '\0';

            // calculate MD5 checksum
            unsigned char md5_hash[16];
            memset(md5_hash, 0, sizeof(unsigned char) * 16);
            MD5_CTX md5ctx;
            MD5_Init(&md5ctx);
            MD5_Update(&md5ctx, kernel_source, kernel_size);
            MD5_Final(md5_hash, &md5ctx);

            // compute compiled kernel file name
            std::stringstream compiled_kernel_name;
            compiled_kernel_name << "/tmp/";
            compiled_kernel_name << std::setbase(16);
            for (int i = 0; i < 16; ++i) {
                compiled_kernel_name << static_cast<unsigned int>(md5_hash[i]);
            }
            compiled_kernel_name << ".bin";

            //std::cout << "Try load precompiled kernel: " << compiled_kernel_name.str() << std::endl;

            // check if pre-compiled kernel binary exist
            std::ifstream precompiled_kernel(compiled_kernel_name.str(), std::ifstream::binary);
            if (precompiled_kernel) {
                // use pre-compiled kernel binary
                precompiled_kernel.seekg(0, std::ios_base::end);
                size_t len = precompiled_kernel.tellg();
                precompiled_kernel.seekg(0, std::ios_base::beg);
                //std::cout << "Length of precompiled kernel: " << len << std::endl;
                unsigned char* compiled_kernel = new unsigned char[len];
                precompiled_kernel.read(reinterpret_cast<char*>(compiled_kernel), len);
                precompiled_kernel.close();

                const unsigned char *ks = (const unsigned char *)compiled_kernel;
                program = clCreateProgramWithBinary(context, 1, &device, &len, &ks, NULL, &err);
                if (err == CL_SUCCESS)
                    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
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
                delete [] compiled_kernel;
    
            } else {
                // pre-compiled kernel binary doesn't exist
                // call CL compiler

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
    
                //Get the number of devices attached with program object
                cl_uint nDevices = 0;
                clGetProgramInfo(program, CL_PROGRAM_NUM_DEVICES, sizeof(cl_uint),&nDevices, NULL);
                assert(nDevices == 1);
    
                //Get the Id of all the attached devices
                cl_device_id *devices = new cl_device_id[nDevices];
                clGetProgramInfo(program, CL_PROGRAM_DEVICES, sizeof(cl_device_id) * nDevices, devices, NULL);
    
                // Get the sizes of all the binary objects
                size_t *pgBinarySizes = new size_t[nDevices];
                clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t) * nDevices, pgBinarySizes, NULL);
    
                // Allocate storage for each binary objects
                unsigned char **pgBinaries = new unsigned char*[nDevices];
                for (cl_uint i = 0; i < nDevices; i++)
                {
                    pgBinaries[i] = new unsigned char[pgBinarySizes[i]];
                }
    
                // Get all the binary objects
                clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char*) * nDevices, pgBinaries, NULL);
    
                // save compiled kernel binary
                std::ofstream compiled_kernel(compiled_kernel_name.str(), std::ostream::binary);
                compiled_kernel.write(reinterpret_cast<const char*>(pgBinaries[0]), pgBinarySizes[0]);
                compiled_kernel.close();
    
                //std::cout << "Kernel written to: " << compiled_kernel_name.str() << std::endl;
    
                // release memory
                for (cl_uint i = 0; i < nDevices; ++i) {
                    delete [] pgBinaries[i];
                }
                delete [] pgBinaries;
                delete [] pgBinarySizes;
                delete [] devices;

            } // if (precompiled_kernel) 

            __mcw_cxxamp_compiled[device] = true;
            free(kernel_source);
            // FIXME: MatchKernelNames is temporarily commented out for better purpose
            // therefore no need to call the following
            #if 0
            getKernelNames(program);
            #endif
#endif
        }
    }

} // namespce CLAMP
} // namespace Concurrency
#if defined(CXXAMP_ENABLE_HSA)
#include "HSAContext.h"
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

static std::map<std::string, HSAContext::Kernel *> __mcw_hsa_kernels;
void *CreateHSAKernel(std::string s)
{
  HSAContext::Kernel *kernel = __mcw_hsa_kernels[s];
  if (!kernel) {
      size_t kernel_size = (size_t)((void *)kernel_size_);
      char *kernel_source = (char*)malloc(kernel_size+1);
      memcpy(kernel_source, kernel_source_, kernel_size);
      kernel_source[kernel_size] = '\0';
      std::string kname = std::string("&")+s;
      //std::cerr << "CLAMP::HSA::Creating kernel: " << kname << "\n";
      kernel = GetOrInitHSAContext()->
          createKernel(kernel_source, kernel_size, kname.c_str());
      if (!kernel) {
          std::cerr << "CLAMP::HSA: Unable to create kernel\n";
          abort();
      } else {
          //std::cerr << "CLAMP::HSA: Created kernel\n";
      }
      __mcw_hsa_kernels[s] = kernel;
  }

  HSAContext::Dispatch *dispatch = GetOrInitHSAContext()->createDispatch(kernel);
  dispatch->clearArgs();
//#define CXXAMP_ENABLE_HSAIL_HLC_DEVELOPMENT_COMPILER 1
#ifndef CXXAMP_ENABLE_HSAIL_HLC_DEVELOPMENT_COMPILER
  dispatch->pushLongArg(0);
  dispatch->pushLongArg(0);
  dispatch->pushLongArg(0);
  dispatch->pushLongArg(0);
  dispatch->pushLongArg(0);
  dispatch->pushLongArg(0);
#endif
  return dispatch;
}

namespace HSA {
void RegisterMemory(void *p, size_t sz)
{
    //std::cerr << "registering: ptr " << p << " of size " << sz << "\n";
    GetOrInitHSAContext()->registerArrayMemory(p, sz);
}
}

std::future<void> HSALaunchKernelAsync(void *ker, size_t nr_dim, size_t *global, size_t *local)
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
  dispatch->setLaunchAttributes(nr_dim, global, local);
  //std::cerr << "Now real launch\n";
  //kernel->dispatchKernelWaitComplete();

  return dispatch->dispatchKernelAndGetFuture();
}

void HSALaunchKernel(void *ker, size_t nr_dim, size_t *global, size_t *local)
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
  dispatch->setLaunchAttributes(nr_dim, global, local);
  //std::cerr << "Now real launch\n";
  dispatch->dispatchKernelWaitComplete();
}

void HSAPushArg(void *ker, size_t sz, const void *v)
{
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
void HSAPushPointer(void *ker, void *val)
{
    //std::cerr << "pushing:" << ker << " of ptr " << val << "\n";
    HSAContext::Dispatch *dispatch =
        reinterpret_cast<HSAContext::Dispatch*>(ker);
    dispatch->pushPointerArg(val);
}
} // namespace CLAMP
} // namespace Concurrency
#endif
