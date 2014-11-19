####################
# OpenCL headers
####################
if (HAS_OPENCL EQUAL 1)
  include_directories(${OPENCL_HEADER}/../)
endif (HAS_OPENCL EQUAL 1)

####################
# C++AMP runtime (OpenCL zero-copy implementation)
####################
if ((HAS_OPENCL EQUAL 1) AND (HAS_OPENCL20 EQUAL 1))
add_mcwamp_library_opencl(mcwamp_opencl_20 mcwamp_opencl_20.cpp)
install(TARGETS mcwamp_opencl_20
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
MESSAGE(STATUS "OpenCL 2.0 available, going to build OpenCL (SVM) C++AMP Runtime")
else ((HAS_OPENCL EQUAL 1) AND (HAS_OPENCL20 EQUAL 1))
MESSAGE(STATUS "OpenCL 2.0 NOT available, NOT going to build OpenCL (SVM) C++AMP Runtime")
endif ((HAS_OPENCL EQUAL 1) AND (HAS_OPENCL20 EQUAL 1))

