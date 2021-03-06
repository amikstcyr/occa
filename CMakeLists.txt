cmake_minimum_required (VERSION 3.0 FATAL_ERROR)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

message(STATUS "Using CMake version ${CMAKE_VERSION}")
cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0056 NEW)

OPTION(USE_INTEL "Use Intel compilers" OFF)

IF (USE_INTEL)
    message("Using Intel compilers")
    include(IntelCompilers)
ENDIF()

# Set project
project (occa)

include(OS)

OPTION(ALLOW_CUDA "Allow CUDA" OFF)
OPTION(ALLOW_MPI  "Allow MPI" OFF)
OPTION(ALLOW_OPENMP "Allow OpenMP" OFF)
OPTION(ALLOW_OPENCL "Allow OpenCL" OFF)
OPTION(ALLOW_METAL "Allow Metal" OFF)
OPTION(ALLOW_HIP "Allow HIP" OFF)
#OPTION(ALLOW_ONEAPI "Allow OneAPI" OFF)

SET(PROJ_DIR ${CMAKE_CURRENT_SOURCE_DIR})

link_directories(${PROJ_DIR}/lib)

find_package(Threads REQUIRED)
SET(CUDA_USE_STATIC_CUDA_RUNTIME OFF)

IF (ALLOW_OPENMP)
    INCLUDE(OpenMP)
ENDIF()

IF (ALLOW_OPENCL)
    INCLUDE(OpenCL)
ENDIF()

# searches for CUDA if allowed
IF (ALLOW_CUDA)
    IF(${CMAKE_VERSION} VERSION_LESS "3.8.0")
        include(CUDA)
      ELSE()
        set(OCCA_CUDA_ENABLED 1)
        set(WITH_CUDA 1)
        enable_language(CUDA)
    ENDIF()
ENDIF()

# searches for MPI if allowed
IF (ALLOW_MPI)
    include(MPI)
ENDIF()

IF (ALLOW_METAL)
    include(Metal)
ENDIF()

IF (ALLOW_HIP)
#    set(OCCA_HIP_ENABLED 1)
#    set(WITH_HIP 1)
#    INCLUDE(ROCm) ... not available, TODO occa/cmake/FindROCm.cmake
ENDIF()

add_definitions(-DUSE_CMAKE)

include_directories(${PROJ_DIR}/include)

SET(CMAKE_POSITION_INDEPENDENT_CODE ON)

IF (NOT USE_INTEL)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-c++11-long-long")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++11-long-long")
ENDIF()
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")

include_directories(${PROJECT_BINARY_DIR})
SET(OCCA_BUILD_DIR ${PROJECT_BINARY_DIR})
# TODO:
CONFIGURE_FILE( scripts/compiledDefinesTemplate.hpp.in ${PROJECT_BINARY_DIR}/occa/defines/compiledDefines.hpp )
# will not work because build directory unknown to runtime compiler
# CONFIGURE_FILE( scripts/compiledDefinesTemplate.hpp.in ${PROJECT_SOURCE_DIR}/include/occa/defines/compiledDefines.hpp )
install(FILES ${PROJECT_BINARY_DIR}/occa/defines/compiledDefines.hpp DESTINATION include/occa/defines)

add_subdirectory(src)
add_subdirectory(bin)
# add_subdirectory(tests)
add_subdirectory(examples)

install(DIRECTORY include/ DESTINATION include)
