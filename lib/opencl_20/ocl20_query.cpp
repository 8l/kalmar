#include <iostream>
#include <cassert>
#include <cstring>
#include <CL/cl.h>
#include <CL/cl_ext.h>


#define MAX_PLATFORM_COUNT (16)
#define MAX_PLATFORM_INFO_LENGTH (4096)

#define MAX_DEVICE_COUNT (64)
#define MAX_DEVICE_INFO_LENGTH (16384)

#define MAX_WORK_ITEM_DIMENSION_COUNT (8)

#define MAX_DEVICE_PARTITION_PROPERTY_COUNT (32)

#define MAX_COMMAND_QUEUE_PROPERTIES_COUNT (8)

class MyCL {
public:
  MyCL() {}

  static inline void getDeviceInfoImpl(cl_device_id device_id, cl_device_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret) {
    cl_int err;
    err = clGetDeviceInfo(device_id, param_name, param_value_size, param_value, param_value_size_ret);
    assert(err == CL_SUCCESS);
  }

  static void getDeviceInfo(int pid, int did, const char* device_info_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret) {
    cl_uint num_platforms;
    cl_int err;
    cl_platform_id platform_id[MAX_PLATFORM_COUNT];
    cl_uint num_devices;
    cl_device_id device_id[MAX_DEVICE_COUNT];

    err = clGetPlatformIDs(MAX_PLATFORM_COUNT, platform_id, &num_platforms);
    assert(err == CL_SUCCESS);
    assert((pid >= 0) && (pid < num_platforms));

    err = clGetDeviceIDs(platform_id[pid], CL_DEVICE_TYPE_ALL, MAX_DEVICE_COUNT, device_id, &num_devices);
    assert(err == CL_SUCCESS);
    assert((did >= 0) && (did < num_devices));

    if (!strcmp(device_info_name, "CL_DEVICE_ADDRESS_BITS")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_ADDRESS_BITS, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_AVAILABLE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_AVAILABLE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_BUILT_IN_KERNELS")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_BUILT_IN_KERNELS, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_COMPILER_AVAILABLE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_COMPILER_AVAILABLE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_DOUBLE_FP_CONFIG")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_DOUBLE_FP_CONFIG, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_ENDIAN_LITTLE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_ENDIAN_LITTLE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_ERROR_CORRECTION_SUPPORT")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_ERROR_CORRECTION_SUPPORT, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_EXECUTION_CAPABILITIES")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_EXECUTION_CAPABILITIES, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_EXTENSIONS")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_EXTENSIONS, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_GLOBAL_MEM_CACHE_SIZE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_GLOBAL_MEM_CACHE_TYPE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_GLOBAL_MEM_CACHE_TYPE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_GLOBAL_MEM_SIZE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_GLOBAL_MEM_SIZE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_GLOBAL_VARIABLE_PREFERRED_TOTAL_SIZE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_GLOBAL_VARIABLE_PREFERRED_TOTAL_SIZE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_IMAGE2D_MAX_HEIGHT")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_IMAGE2D_MAX_HEIGHT, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_IMAGE2D_MAX_WIDTH")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_IMAGE2D_MAX_WIDTH, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_IMAGE3D_MAX_DEPTH")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_IMAGE3D_MAX_DEPTH, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_IMAGE3D_MAX_HEIGHT")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_IMAGE3D_MAX_HEIGHT, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_IMAGE3D_MAX_WIDTH")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_IMAGE3D_MAX_WIDTH, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_IMAGE_MAX_ARRAY_SIZE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_IMAGE_MAX_ARRAY_SIZE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_IMAGE_MAX_BUFFER_SIZE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_IMAGE_MAX_BUFFER_SIZE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_IMAGE_PITCH_ALIGNMENT")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_IMAGE_PITCH_ALIGNMENT, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_IMAGE_SUPPORT")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_IMAGE_SUPPORT, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_LINKER_AVAILABLE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_LINKER_AVAILABLE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_LOCAL_MEM_SIZE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_LOCAL_MEM_SIZE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_LOCAL_MEM_TYPE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_LOCAL_MEM_TYPE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_MAX_CLOCK_FREQUENCY")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_MAX_CLOCK_FREQUENCY, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_MAX_COMPUTE_UNITS")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_MAX_COMPUTE_UNITS, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_MAX_CONSTANT_ARGS")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_MAX_CONSTANT_ARGS, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_MAX_MEM_ALLOC_SIZE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_MAX_MEM_ALLOC_SIZE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_MAX_ON_DEVICE_EVENTS")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_MAX_ON_DEVICE_EVENTS, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_MAX_ON_DEVICE_QUEUES")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_MAX_ON_DEVICE_QUEUES, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_MAX_PARAMETER_SIZE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_MAX_PARAMETER_SIZE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_MAX_PIPE_ARGS")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_MAX_PIPE_ARGS, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_MAX_READ_IMAGE_ARGS")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_MAX_READ_IMAGE_ARGS, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_MAX_READ_WRITE_IMAGE_ARGS")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_MAX_READ_WRITE_IMAGE_ARGS, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_MAX_SAMPLERS")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_MAX_SAMPLERS, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_MAX_WORK_GROUP_SIZE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_MAX_WORK_GROUP_SIZE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_MAX_WORK_ITEM_SIZES")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_MAX_WORK_ITEM_SIZES, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_MAX_WRITE_IMAGE_ARGS")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_MAX_WRITE_IMAGE_ARGS, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_MEM_BASE_ADDR_ALIGN")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_MEM_BASE_ADDR_ALIGN, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_NAME")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_NAME, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_NATIVE_VECTOR_WIDTH_INT")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_NATIVE_VECTOR_WIDTH_INT, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_OPENCL_C_VERSION")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_OPENCL_C_VERSION, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_PARENT_DEVICE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_PARENT_DEVICE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_PARTITION_AFFINITY_DOMAIN")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_PARTITION_AFFINITY_DOMAIN, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_PARTITION_MAX_SUB_DEVICES")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_PARTITION_MAX_SUB_DEVICES, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_PARTITION_PROPERTIES")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_PARTITION_PROPERTIES, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_PARTITION_TYPE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_PARTITION_TYPE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_PIPE_MAX_ACTIVE_RESERVATIONS")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_PIPE_MAX_ACTIVE_RESERVATIONS, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_PIPE_MAX_PACKET_SIZE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_PIPE_MAX_PACKET_SIZE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_PLATFORM")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_PLATFORM, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_PREFERRED_GLOBAL_ATOMIC_ALIGNMENT")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_PREFERRED_GLOBAL_ATOMIC_ALIGNMENT, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_PREFERRED_INTEROP_USER_SYNC")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_PREFERRED_INTEROP_USER_SYNC, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_PREFERRED_LOCAL_ATOMIC_ALIGNMENT")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_PREFERRED_LOCAL_ATOMIC_ALIGNMENT, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_PREFERRED_PLATFORM_ATOMIC_ALIGNMENT")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_PREFERRED_PLATFORM_ATOMIC_ALIGNMENT, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_PRINTF_BUFFER_SIZE")) {
      getDeviceInfoImpl(device_id[did],  CL_DEVICE_PRINTF_BUFFER_SIZE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_PROFILE")) {
      getDeviceInfoImpl(device_id[did],  CL_DEVICE_PROFILE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_PROFILING_TIMER_RESOLUTION")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_PROFILING_TIMER_RESOLUTION, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE")) {
      getDeviceInfoImpl(device_id[did],  CL_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_QUEUE_ON_DEVICE_PROPERTIES")) {
      getDeviceInfoImpl(device_id[did],  CL_DEVICE_QUEUE_ON_DEVICE_PROPERTIES, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_QUEUE_ON_HOST_PROPERTIES")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_QUEUE_ON_HOST_PROPERTIES, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_REFERENCE_COUNT")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_REFERENCE_COUNT, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_SINGLE_FP_CONFIG")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_SINGLE_FP_CONFIG, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_SPIR_VERSIONS")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_SPIR_VERSIONS, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_SVM_CAPABILITIES")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_SVM_CAPABILITIES, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_TERMINATE_CAPABILITY_KHR")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_TERMINATE_CAPABILITY_KHR, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_TYPE")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_TYPE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_VENDOR")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_VENDOR, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_VENDOR_ID")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_VENDOR_ID, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DEVICE_VERSION")) {
      getDeviceInfoImpl(device_id[did], CL_DEVICE_VERSION, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(device_info_name, "CL_DRIVER_VERSION")) {
      getDeviceInfoImpl(device_id[did], CL_DRIVER_VERSION, param_value_size, param_value, param_value_size_ret);
    }
  }

  static inline void getPlatformInfoImpl(cl_platform_id platform_id, cl_platform_info platform_info, size_t param_value_size, char* param_value, size_t* param_value_size_ret) {
    cl_int err;
    err = clGetPlatformInfo(platform_id, platform_info, param_value_size, param_value, param_value_size_ret);
    assert(err == CL_SUCCESS);
  }

  static void getPlatformInfo(int id, const char* platform_info_name, size_t param_value_size, char* param_value, size_t* param_value_size_ret) {
    cl_uint num_platforms;
    cl_int err;
    cl_platform_id platform_id[MAX_PLATFORM_COUNT];

    err = clGetPlatformIDs(MAX_PLATFORM_COUNT, platform_id, &num_platforms);
    assert(err == CL_SUCCESS);
    assert((id >= 0) && (id < num_platforms));

    if (!strcmp(platform_info_name, "CL_PLATFORM_PROFILE"))  {
      getPlatformInfoImpl(platform_id[id], CL_PLATFORM_PROFILE, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(platform_info_name, "CL_PLATFORM_VERSION")) {
      getPlatformInfoImpl(platform_id[id], CL_PLATFORM_VERSION, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(platform_info_name, "CL_PLATFORM_NAME")) {
      getPlatformInfoImpl(platform_id[id], CL_PLATFORM_NAME, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(platform_info_name, "CL_PLATFORM_VENDOR")) {
      getPlatformInfoImpl(platform_id[id], CL_PLATFORM_VENDOR, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(platform_info_name, "CL_PLATFORM_EXTENSIONS")) {
      getPlatformInfoImpl(platform_id[id], CL_PLATFORM_EXTENSIONS, param_value_size, param_value, param_value_size_ret);
    } else if (!strcmp(platform_info_name, "CL_PLATFORM_ICD_SUFFIX_KHR")) {
      getPlatformInfoImpl(platform_id[id], CL_PLATFORM_ICD_SUFFIX_KHR, param_value_size, param_value, param_value_size_ret);
    }
  }

  static int enumeratePlatforms() {
    cl_uint num_platforms;
    cl_int err;
    err = clGetPlatformIDs(0, NULL, &num_platforms);
    assert(err == CL_SUCCESS);
    return num_platforms;
  }

  static int enumerateDevices(int id) {
    cl_uint num_platforms;
    cl_int err;
    cl_platform_id platform_id[MAX_PLATFORM_COUNT];
    cl_uint num_devices;

    err = clGetPlatformIDs(MAX_PLATFORM_COUNT, platform_id, &num_platforms);
    assert(err == CL_SUCCESS);
    assert((id >= 0) && (id < num_platforms));

    err = clGetDeviceIDs(platform_id[id], CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices);
    assert(err == CL_SUCCESS);
    return num_devices;
  }

private:
  cl_platform_id	platform;
  cl_context	 	context;
  cl_device_id	 	device;
  cl_command_queue	queue;
  cl_program 		program;
  cl_kernel 		kernel;
};


int main() {
  // enumerate CL platforms
  int num_platforms = MyCL::enumeratePlatforms();

#define DUMP_PLATFORM_INFO (1)
#if DUMP_PLATFORM_INFO
  // fetch individual CL platform info
  char* platform_info_cstr = new char[MAX_PLATFORM_INFO_LENGTH];
  assert(platform_info_cstr);
  size_t platform_info_cstr_length;
  for (int i = 0; i < num_platforms; ++i) {
    std::cout << "Platform #" << i << std::endl;

    MyCL::getPlatformInfo(i, "CL_PLATFORM_PROFILE", MAX_PLATFORM_INFO_LENGTH, platform_info_cstr, &platform_info_cstr_length);
    std::cout << "CL_PLATFORM_PROFILE: " << platform_info_cstr << std::endl;

    MyCL::getPlatformInfo(i, "CL_PLATFORM_VERSION", MAX_PLATFORM_INFO_LENGTH, platform_info_cstr, &platform_info_cstr_length);
    std::cout << "CL_PLATFORM_VERSION: " << platform_info_cstr << std::endl;

    MyCL::getPlatformInfo(i, "CL_PLATFORM_NAME", MAX_PLATFORM_INFO_LENGTH, platform_info_cstr, &platform_info_cstr_length);
    std::cout << "CL_PLATFORM_NAME: " << platform_info_cstr << std::endl;

    MyCL::getPlatformInfo(i, "CL_PLATFORM_VENDOR", MAX_PLATFORM_INFO_LENGTH, platform_info_cstr, &platform_info_cstr_length);
    std::cout << "CL_PLATFORM_VENDOR: " << platform_info_cstr << std::endl;

    MyCL::getPlatformInfo(i, "CL_PLATFORM_EXTENSIONS", MAX_PLATFORM_INFO_LENGTH, platform_info_cstr, &platform_info_cstr_length);
    std::cout << "CL_PLATFORM_EXTENSIONS: " << platform_info_cstr << std::endl;

    MyCL::getPlatformInfo(i, "CL_PLATFORM_ICD_SUFFIX_KHR", MAX_PLATFORM_INFO_LENGTH, platform_info_cstr, &platform_info_cstr_length);
    std::cout << "CL_PLATFORM_ICD_SUFFIX_KHR: " << platform_info_cstr << std::endl;
  }
  delete [] platform_info_cstr;
#endif

#define DUMP_DEVICE_INFO (1)
#if DUMP_DEVICE_INFO
  // for each CL platform
  // enumerate CL devices
  for (int i = 0; i < num_platforms; ++i) {
    int num_devices = MyCL::enumerateDevices(i);
    std::cout << "Platform #" << i << ": " << num_devices << " devices\n";

    cl_uint ret_uint;
    cl_bool ret_bool;
    char* device_info_cstr = new char[MAX_DEVICE_INFO_LENGTH];
    assert(device_info_cstr);
    cl_device_fp_config ret_device_fp_config;
    cl_device_exec_capabilities ret_device_exec_capabilities;
    cl_ulong ret_ulong;
    cl_device_mem_cache_type ret_device_mem_cache_type;
    size_t ret_size_t;
    cl_device_local_mem_type ret_device_local_mem_type;
    size_t ret_device_max_work_item_sizes[MAX_WORK_ITEM_DIMENSION_COUNT];
    cl_device_id ret_device_id;
    cl_device_affinity_domain ret_device_affinity_domain;
    cl_device_partition_property ret_device_partition_property[MAX_DEVICE_PARTITION_PROPERTY_COUNT];
    cl_platform_id ret_platform_id;
    cl_command_queue_properties ret_command_queue_properties[MAX_COMMAND_QUEUE_PROPERTIES_COUNT];
    cl_device_svm_capabilities ret_device_svm_capabilities;
    // although defined in OpenCL 2.0, NOT implemented in AMD driver version 14.41
    //cl_device_terminate_capability_khr ret_device_terminate_capability_khr;
    cl_device_type ret_device_type;

    size_t ret_size;
    for (int j = 0; j < num_devices; ++j) {
      std::cout << "Platform #" << i << " Device #" << j << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_ADDRESS_BITS", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_ADDRESS_BITS: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_AVAILABLE", sizeof(cl_bool), &ret_bool, &ret_size);
      std::cout << "CL_DEVICE_AVAILABLE: " << ret_bool << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_BUILT_IN_KERNELS", MAX_DEVICE_INFO_LENGTH, device_info_cstr, &ret_size);
      std::cout << "CL_DEVICE_BUILT_IN_KERNELS: " << device_info_cstr << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_DOUBLE_FP_CONFIG", sizeof(cl_device_fp_config), &ret_device_fp_config, &ret_size);
      std::cout << "CL_DEVICE_DOUBLE_FP_CONFIG: " << ret_device_fp_config << std::endl;
      if (ret_device_fp_config & CL_FP_DENORM) {
        std::cout << "CL_DEVICE_DOUBLE_FP_CONFIG: CL_FP_DENORM" << std::endl;
      }
      if (ret_device_fp_config & CL_FP_INF_NAN) {
        std::cout << "CL_DEVICE_DOUBLE_FP_CONFIG: CL_FP_INF_NAN" << std::endl;
      }
      if (ret_device_fp_config & CL_FP_ROUND_TO_NEAREST) {
        std::cout << "CL_DEVICE_DOUBLE_FP_CONFIG: CL_FP_ROUND_TO_NEAREST" << std::endl;
      }
      if (ret_device_fp_config & CL_FP_ROUND_TO_ZERO) {
        std::cout << "CL_DEVICE_DOUBLE_FP_CONFIG: CL_FP_ROUND_TO_ZERO" << std::endl;
      }
      if (ret_device_fp_config & CL_FP_ROUND_TO_INF) {
        std::cout << "CL_DEVICE_DOUBLE_FP_CONFIG: CL_FP_ROUND_TO_INF" << std::endl;
      }
      if (ret_device_fp_config & CL_FP_FMA) {
        std::cout << "CL_DEVICE_DOUBLE_FP_CONFIG: CL_FP_FMA" << std::endl;
      }
      if (ret_device_fp_config & CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT) {
        std::cout << "CL_DEVICE_DOUBLE_FP_CONFIG: CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT" << std::endl;
      }
      if (ret_device_fp_config & CL_FP_SOFT_FLOAT) {
        std::cout << "CL_DEVICE_DOUBLE_FP_CONFIG: CL_FP_SOFT_FLOAT" << std::endl;
      }

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_ENDIAN_LITTLE", sizeof(cl_bool), &ret_bool, &ret_size);
      std::cout << "CL_DEVICE_ENDIAN_LITTLE: " << ret_bool << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_ERROR_CORRECTION_SUPPORT", sizeof(cl_bool), &ret_bool, &ret_size);
      std::cout << "CL_DEVICE_ERROR_CORRECTION_SUPPORT: " << ret_bool << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_EXECUTION_CAPABILITIES", sizeof(cl_device_exec_capabilities), &ret_device_exec_capabilities, &ret_size);
      std::cout << "CL_DEVICE_EXECUTION_CAPABILITIES: " << ret_device_exec_capabilities << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_EXTENSIONS", MAX_DEVICE_INFO_LENGTH, device_info_cstr, &ret_size);
      std::cout << "CL_DEVICE_EXTENSIONS: " << device_info_cstr << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_GLOBAL_MEM_CACHE_SIZE", sizeof(cl_ulong), &ret_ulong, &ret_size);
      std::cout << "CL_DEVICE_GLOBAL_MEM_CACHE_SIZE: " << ret_ulong << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_GLOBAL_MEM_CACHE_TYPE", sizeof(cl_device_mem_cache_type), &ret_device_mem_cache_type, &ret_size);
      std::cout << "CL_DEVICE_GLOBAL_MEM_CACHE_TYPE: " << ret_device_mem_cache_type << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_GLOBAL_MEM_SIZE", sizeof(cl_ulong), &ret_ulong, &ret_size);
      std::cout << "CL_DEVICE_GLOBAL_MEM_SIZE: " << ret_ulong << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_GLOBAL_VARIABLE_PREFERRED_TOTAL_SIZE", sizeof(size_t), &ret_size_t, &ret_size);
      std::cout << "CL_DEVICE_GLOBAL_VARIABLE_PREFERRED_TOTAL_SIZE: " << ret_size_t << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_IMAGE2D_MAX_HEIGHT", sizeof(size_t), &ret_size_t, &ret_size);
      std::cout << "CL_DEVICE_IMAGE2D_MAX_HEIGHT: " << ret_size_t << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_IMAGE2D_MAX_WIDTH", sizeof(size_t), &ret_size_t, &ret_size);
      std::cout << "CL_DEVICE_IMAGE2D_MAX_WIDTH: " << ret_size_t << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_IMAGE3D_MAX_DEPTH", sizeof(size_t), &ret_size_t, &ret_size);
      std::cout << "CL_DEVICE_IMAGE3D_MAX_DEPTH: " << ret_size_t << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_IMAGE3D_MAX_HEIGHT", sizeof(size_t), &ret_size_t, &ret_size);
      std::cout << "CL_DEVICE_IMAGE3D_MAX_HEIGHT: " << ret_size_t << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_IMAGE3D_MAX_WIDTH", sizeof(size_t), &ret_size_t, &ret_size);
      std::cout << "CL_DEVICE_IMAGE3D_MAX_WIDTH: " << ret_size_t << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_IMAGE_MAX_ARRAY_SIZE", sizeof(size_t), &ret_size_t, &ret_size);
      std::cout << "CL_DEVICE_IMAGE_MAX_ARRAY_SIZE: " << ret_size_t << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_IMAGE_MAX_BUFFER_SIZE", sizeof(size_t), &ret_size_t, &ret_size);
      std::cout << "CL_DEVICE_IMAGE_MAX_BUFFER_SIZE: " << ret_size_t << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_IMAGE_PITCH_ALIGNMENT", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_IMAGE_PITCH_ALIGNMENT: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_IMAGE_SUPPORT", sizeof(cl_bool), &ret_bool, &ret_size);
      std::cout << "CL_DEVICE_IMAGE_SUPPORT: " << ret_bool << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_LINKER_AVAILABLE", sizeof(cl_bool), &ret_bool, &ret_size);
      std::cout << "CL_DEVICE_LINKER_AVAILABLE: " << ret_bool << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_LOCAL_MEM_SIZE", sizeof(cl_ulong), &ret_ulong, &ret_size);
      std::cout << "CL_DEVICE_LOCAL_MEM_SIZE: " << ret_ulong << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_LOCAL_MEM_TYPE", sizeof(cl_device_local_mem_type), &ret_device_local_mem_type, &ret_size);
      std::cout << "CL_DEVICE_LOCAL_MEM_TYPE: " << ret_device_local_mem_type<< std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_MAX_CLOCK_FREQUENCY", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_MAX_CLOCK_FREQUENCY: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_MAX_COMPUTE_UNITS", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_MAX_COMPUTE_UNITS: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_MAX_CONSTANT_ARGS", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_MAX_CONSTANT_ARGS: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE", sizeof(cl_ulong), &ret_ulong, &ret_size);
      std::cout << "CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE: " << ret_ulong << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE", sizeof(size_t), &ret_size_t, &ret_size);
      std::cout << "CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE: " << ret_size_t << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_MAX_MEM_ALLOC_SIZE", sizeof(cl_ulong), &ret_ulong, &ret_size);
      std::cout << "CL_DEVICE_MAX_MEM_ALLOC_SIZE: " << ret_ulong << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_MAX_ON_DEVICE_EVENTS", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_MAX_ON_DEVICE_EVENTS: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_MAX_ON_DEVICE_QUEUES", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_MAX_ON_DEVICE_QUEUES: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_MAX_PARAMETER_SIZE", sizeof(size_t), &ret_size_t, &ret_size);
      std::cout << "CL_DEVICE_MAX_PARAMETER_SIZE: " << ret_size_t << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_MAX_PIPE_ARGS", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_MAX_PIPE_ARGS: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_MAX_READ_IMAGE_ARGS", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_MAX_READ_IMAGE_ARGS: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_MAX_READ_WRITE_IMAGE_ARGS", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_MAX_READ_WRITE_IMAGE_ARGS: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_MAX_SAMPLERS", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_MAX_SAMPLERS: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_MAX_WORK_GROUP_SIZE", sizeof(size_t), &ret_size_t, &ret_size);
      std::cout << "CL_DEVICE_MAX_WORK_GROUP_SIZE: " << ret_size_t << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_MAX_WORK_ITEM_SIZES", sizeof(size_t) * MAX_WORK_ITEM_DIMENSION_COUNT, &ret_device_max_work_item_sizes, &ret_size);
      for (size_t k = 0; k < ret_size / sizeof(size_t); ++k) {
        std::cout << "CL_DEVICE_MAX_WORK_ITEM_SIZES[" << k << "]: " << ret_device_max_work_item_sizes[k] << std::endl;
      }

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_MAX_WRITE_IMAGE_ARGS", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_MAX_WRITE_IMAGE_ARGS: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_MEM_BASE_ADDR_ALIGN", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_MEM_BASE_ADDR_ALIGN: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_NAME", MAX_DEVICE_INFO_LENGTH, device_info_cstr, &ret_size);
      std::cout << "CL_DEVICE_NAME: " << device_info_cstr << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_NATIVE_VECTOR_WIDTH_INT", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_NATIVE_VECTOR_WIDTH_INT: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_OPENCL_C_VERSION", MAX_DEVICE_INFO_LENGTH, device_info_cstr, &ret_size);
      std::cout << "CL_DEVICE_OPENCL_C_VERSION: " << device_info_cstr << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_PARENT_DEVICE", sizeof(cl_device_id), &ret_device_id, &ret_size);
      std::cout << "CL_DEVICE_PARENT_DEVICE: " << ret_device_id << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_PARTITION_AFFINITY_DOMAIN", sizeof(cl_device_affinity_domain), &ret_device_affinity_domain, &ret_size);
      std::cout << "CL_DEVICE_PARTITION_AFFINITY_DOMAIN: " << ret_device_affinity_domain << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_PARTITION_MAX_SUB_DEVICES", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_PARTITION_MAX_SUB_DEVICES: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_PARTITION_PROPERTIES", sizeof(cl_device_partition_property) * MAX_DEVICE_PARTITION_PROPERTY_COUNT, &ret_device_partition_property, &ret_size);
      for (size_t k = 0; k < ret_size / sizeof(cl_device_partition_property); ++k) {
        std::cout << "CL_DEVICE_PARTITION_PROPERTIES[" << k << "]: " << ret_device_partition_property[k] << std::endl;
      }

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_PARTITION_TYPE", sizeof(cl_device_partition_property) * MAX_DEVICE_PARTITION_PROPERTY_COUNT, &ret_device_partition_property, &ret_size);
      for (size_t k = 0; k < ret_size / sizeof(cl_device_partition_property); ++k) {
        std::cout << "CL_DEVICE_PARTITION_TYPE[" << k << "]: " << ret_device_partition_property[k] << std::endl;
      }

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_PIPE_MAX_ACTIVE_RESERVATIONS", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_PIPE_MAX_ACTIVE_RESERVATIONS: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_PIPE_MAX_PACKET_SIZE", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_PIPE_MAX_PACKET_SIZE: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_PLATFORM", sizeof(cl_platform_id), &ret_platform_id, &ret_size);
      std::cout << "CL_DEVICE_PLATFORM: " << ret_platform_id << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_PREFERRED_GLOBAL_ATOMIC_ALIGNMENT", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_PREFERRED_GLOBAL_ATOMIC_ALIGNMENT: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_PREFERRED_INTEROP_USER_SYNC", sizeof(cl_bool), &ret_bool, &ret_size);
      std::cout << "CL_DEVICE_PREFERRED_INTEROP_USER_SYNC: " << ret_bool << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_PREFERRED_LOCAL_ATOMIC_ALIGNMENT", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_PREFERRED_LOCAL_ATOMIC_ALIGNMENT: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_PREFERRED_PLATFORM_ATOMIC_ALIGNMENT", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_PREFERRED_PLATFORM_ATOMIC_ALIGNMENT: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_PRINTF_BUFFER_SIZE", sizeof(size_t), &ret_size_t, &ret_size);
      std::cout << "CL_DEVICE_PRINTF_BUFFER_SIZE: " << ret_size_t << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_PROFILE", MAX_DEVICE_INFO_LENGTH, device_info_cstr, &ret_size);
      std::cout << "CL_DEVICE_PROFILE: " << device_info_cstr << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_PROFILING_TIMER_RESOLUTION", sizeof(size_t), &ret_size_t, &ret_size);
      std::cout << "CL_DEVICE_PROFILING_TIMER_RESOLUTION: " << ret_size_t << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_QUEUE_ON_DEVICE_PROPERTIES", sizeof(cl_command_queue_properties) * MAX_COMMAND_QUEUE_PROPERTIES_COUNT, &ret_command_queue_properties, &ret_size);
      for (int k = 0; k < ret_size / sizeof(cl_command_queue_properties); ++k) {
        std::cout << "CL_DEVICE_QUEUE_ON_DEVICE_PROPERTIES: " << ret_command_queue_properties[k] << std::endl;
      }

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_QUEUE_ON_HOST_PROPERTIES", sizeof(cl_command_queue_properties) * MAX_COMMAND_QUEUE_PROPERTIES_COUNT, &ret_command_queue_properties, &ret_size);
      for (int k = 0; k < ret_size / sizeof(cl_command_queue_properties); ++k) {
        std::cout << "CL_DEVICE_QUEUE_ON_HOST_PROPERTIES: " << ret_command_queue_properties[k] << std::endl;
      }

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_REFERENCE_COUNT", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_REFERENCE_COUNT: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_SINGLE_FP_CONFIG", sizeof(cl_device_fp_config), &ret_device_fp_config, &ret_size);
      std::cout << "CL_DEVICE_SINGLE_FP_CONFIG: " << ret_device_fp_config << std::endl;
      if (ret_device_fp_config & CL_FP_DENORM) {
        std::cout << "CL_DEVICE_SINGLE_FP_CONFIG: CL_FP_DENORM" << std::endl;
      }
      if (ret_device_fp_config & CL_FP_INF_NAN) {
        std::cout << "CL_DEVICE_SINGLE_FP_CONFIG: CL_FP_INF_NAN" << std::endl;
      }
      if (ret_device_fp_config & CL_FP_ROUND_TO_NEAREST) {
        std::cout << "CL_DEVICE_SINGLE_FP_CONFIG: CL_FP_ROUND_TO_NEAREST" << std::endl;
      }
      if (ret_device_fp_config & CL_FP_ROUND_TO_ZERO) {
        std::cout << "CL_DEVICE_SINGLE_FP_CONFIG: CL_FP_ROUND_TO_ZERO" << std::endl;
      }
      if (ret_device_fp_config & CL_FP_ROUND_TO_INF) {
        std::cout << "CL_DEVICE_SINGLE_FP_CONFIG: CL_FP_ROUND_TO_INF" << std::endl;
      }
      if (ret_device_fp_config & CL_FP_FMA) {
        std::cout << "CL_DEVICE_SINGLE_FP_CONFIG: CL_FP_FMA" << std::endl;
      }
      if (ret_device_fp_config & CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT) {
        std::cout << "CL_DEVICE_SINGLE_FP_CONFIG: CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT" << std::endl;
      }
      if (ret_device_fp_config & CL_FP_SOFT_FLOAT) {
        std::cout << "CL_DEVICE_SINGLE_FP_CONFIG: CL_FP_SOFT_FLOAT" << std::endl;
      }

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_SPIR_VERSIONS", MAX_DEVICE_INFO_LENGTH, device_info_cstr, &ret_size);
      std::cout << "CL_DEVICE_SPIR_VERSIONS: " << device_info_cstr << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_SVM_CAPABILITIES", sizeof(cl_device_svm_capabilities), &ret_device_svm_capabilities, &ret_size);
      std::cout << "CL_DEVICE_SVM_CAPABILITIES: " << ret_device_svm_capabilities << std::endl;
      if (ret_device_svm_capabilities & CL_DEVICE_SVM_COARSE_GRAIN_BUFFER) {
        std::cout << "CL_DEVICE_SVM_CAPABILITIES: CL_DEVICE_SVM_COARSE_GRAIN_BUFFER" << std::endl;
      }
      if (ret_device_svm_capabilities & CL_DEVICE_SVM_FINE_GRAIN_BUFFER) {
        std::cout << "CL_DEVICE_SVM_CAPABILITIES: CL_DEVICE_SVM_FINE_GRAIN_BUFFER" << std::endl;
      }
      if (ret_device_svm_capabilities & CL_DEVICE_SVM_FINE_GRAIN_SYSTEM) {
        std::cout << "CL_DEVICE_SVM_CAPABILITIES: CL_DEVICE_SVM_FINE_GRAIN_SYSTEM" << std::endl;
      }
      if (ret_device_svm_capabilities & CL_DEVICE_SVM_ATOMICS) {
        std::cout << "CL_DEVICE_SVM_CAPABILITIES: CL_DEVICE_SVM_ATOMICS" << std::endl;
      }

      // although defined in OpenCL 2.0, it's NOT implemented in AMD OpenCL driver version 14.41
      //MyCL::getDeviceInfo(i, j, "CL_DEVICE_TERMINATE_CAPABILITY_KHR", sizeof(cl_device_terminate_capability_khr), &ret_device_terminate_capability_khr, &ret_size);

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_TYPE", sizeof(cl_device_type), &ret_device_type, &ret_size);
      std::cout << "CL_DEVICE_TYPE: " << ret_device_type << std::endl;
      if (ret_device_type & CL_DEVICE_TYPE_CPU) {
        std::cout << "CL_DEVICE_TYPE: CL_DEVICE_TYPE_CPU" << std::endl;
      }
      if (ret_device_type & CL_DEVICE_TYPE_GPU) {
        std::cout << "CL_DEVICE_TYPE: CL_DEVICE_TYPE_GPU" << std::endl;
      }
      if (ret_device_type & CL_DEVICE_TYPE_ACCELERATOR) {
        std::cout << "CL_DEVICE_TYPE: CL_DEVICE_TYPE_ACCELERATOR" << std::endl;
      }
      if (ret_device_type & CL_DEVICE_TYPE_DEFAULT) {
        std::cout << "CL_DEVICE_TYPE: CL_DEVICE_TYPE_DEFAULT" << std::endl;
      }
      if (ret_device_type & CL_DEVICE_TYPE_CUSTOM) {
        std::cout << "CL_DEVICE_TYPE: CL_DEVICE_TYPE_CUSTOM" << std::endl;
      }

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_VENDOR", MAX_DEVICE_INFO_LENGTH, device_info_cstr, &ret_size);
      std::cout << "CL_DEVICE_VENDOR: " << device_info_cstr << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_VENDOR_ID", sizeof(cl_uint), &ret_uint, &ret_size);
      std::cout << "CL_DEVICE_VENDOR_ID: " << ret_uint << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DEVICE_VERSION", MAX_DEVICE_INFO_LENGTH, device_info_cstr, &ret_size);
      std::cout << "CL_DEVICE_VERSION: " << device_info_cstr << std::endl;

      MyCL::getDeviceInfo(i, j, "CL_DRIVER_VERSION", MAX_DEVICE_INFO_LENGTH, device_info_cstr, &ret_size);
      std::cout << "CL_DRIVER_VERSION: " << device_info_cstr << std::endl;

    }
    delete [] device_info_cstr;
  }
#endif

  return 0;
}
