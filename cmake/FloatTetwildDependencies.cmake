# ##############################################################################
# Prepare dependencies
# ##############################################################################

# Use modern FetchContent for dependency management
include(FetchContent)

# Set a global preference for STATIC libraries over SHARED ones.
#set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

# Set FetchContent properties for better performance
set(FETCHCONTENT_QUIET ON)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# ##############################################################################
# Required libraries
# ##############################################################################

# Sanitizers
if(FLOAT_TETWILD_WITH_SANITIZERS)
    FetchContent_Declare(
        sanitizers-cmake
        GIT_REPOSITORY https://github.com/arsenm/sanitizers-cmake.git
        GIT_TAG        6947cff3a9c9305eb9c16135dd81da3feb4bf87f
    )
    FetchContent_MakeAvailable(sanitizers-cmake)
    list(APPEND CMAKE_MODULE_PATH ${sanitizers-cmake_SOURCE_DIR}/cmake)
    find_package(Sanitizers)
endif()

# CLI11
if(FLOAT_TETWILD_TOPLEVEL_PROJECT AND NOT TARGET CLI11::CLI11)
    FetchContent_Declare(
        cli11
        GIT_REPOSITORY https://github.com/CLIUtils/CLI11
        GIT_TAG v2.5.0
    )
    FetchContent_MakeAvailable(cli11)
endif()

# fmt
if(NOT TARGET fmt::fmt)
    FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt
        GIT_TAG        11.2.0
    )
    FetchContent_MakeAvailable(fmt)
endif()

# spdlog
if(NOT TARGET spdlog::spdlog)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog
        GIT_TAG        v1.15.3
    )
    
    # Configure spdlog to use external fmt
    set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(spdlog)
endif()

# Eigen3 — pre-declare using a tarball before libigl can trigger a slow GitLab
# git clone. FetchContent records eigen as already populated, so libigl's own
# eigen recipe hits the `if(NOT eigen_POPULATED)` guard and returns early.
if(NOT TARGET Eigen3::Eigen)
    FetchContent_Declare(
        eigen
        URL      https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz
        URL_HASH SHA256=8586084f71f9bde545ee7fa6d00288b264a2b7ac3607b974e54d13e7162c1c72
    )
    FetchContent_GetProperties(eigen)
    if(NOT eigen_POPULATED)
        FetchContent_Populate(eigen)
    endif()
    add_library(Eigen3_Eigen INTERFACE)
    add_library(Eigen3::Eigen ALIAS Eigen3_Eigen)
    target_include_directories(Eigen3_Eigen SYSTEM INTERFACE ${eigen_SOURCE_DIR})
endif()

# libigl
if(NOT TARGET igl::core)
    FetchContent_Declare(
        libigl
        URL      https://github.com/libigl/libigl/archive/refs/tags/v2.6.0.tar.gz
        URL_HASH SHA256=fe3bf58571cccbef774947261284ccf6b7fdf04fcab5f7181e31931e42a0b14f
    )
    set(LIBIGL_BUILD_STATIC ON CACHE BOOL "" FORCE)
    set(LIBIGL_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(libigl)
endif()

# Import libigl targets - use FetchContent source directory
list(APPEND CMAKE_MODULE_PATH "${libigl_SOURCE_DIR}/cmake")

# Geogram
if(NOT TARGET geogram::geogram)
    FetchContent_Declare(
        geogram
        GIT_REPOSITORY https://github.com/BrunoLevy/geogram
        GIT_TAG        v1.9.6
        # Only clone the submodules needed for the core library.
        # Skips glfw and imgui (graphics, disabled via GEOGRAM_WITH_GRAPHICS OFF).
        # GIT_SUBMODULES_RECURSE OFF prevents pybind11 inside amgcl from being cloned.
        GIT_SUBMODULES "src/lib/geogram/third_party/amgcl"
                       "src/lib/geogram/third_party/libMeshb"
                       "src/lib/geogram/third_party/rply"
        GIT_SUBMODULES_RECURSE OFF
    )

    # --- Final Recommended Configuration ---
    # Set the platform to force a static build
    if(MSVC)
        set(GEO_PLATFORM "Win-vs-generic")
    elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
        set(GEO_PLATFORM "Darwin-clang")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(GEO_PLATFORM "Linux64-clang")
    else()
        set(GEO_PLATFORM "Linux64-gcc")
    endif()

    set(VORPALINE_PLATFORM ${GEO_PLATFORM})

    set(GEOGRAM_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(GEOGRAM_BUILD_STATIC ON CACHE BOOL "" FORCE)

    set(GEOGRAM_SUB_BUILD ON CACHE BOOL "Building as subproject" FORCE)
    set(GEOGRAM_LIB_ONLY ON CACHE BOOL "Build geogram lib only" FORCE)
    set(GEOGRAM_WITH_GRAPHICS OFF CACHE BOOL "Disable graphics" FORCE)
    set(GEOGRAM_WITH_LUA OFF CACHE BOOL "Disable LUA" FORCE)
    set(GEOGRAM_WITH_EXPLORAGRAM OFF CACHE BOOL "Disable exploragram" FORCE)
    set(GEOGRAM_WITH_LEGACY_NUMERICS OFF CACHE BOOL "Disable legacy numerics" FORCE)
    set(GEOGRAM_WITH_TRIANGLE OFF CACHE BOOL "Disable triangle" FORCE)

    FetchContent_MakeAvailable(geogram)

    # Geogram's Linux-gcc platform unconditionally adds -fopenmp to CMAKE_CXX_FLAGS,
    # defining _OPENMP and causing third-party code (PoissonRecon, OpenNL) to include
    # <omp.h> which may not be installed. Cancel it on the affected targets by appending
    # -fno-openmp, which GCC processes after CMAKE_CXX_FLAGS in the compile command.
    if((CMAKE_SYSTEM_NAME MATCHES "Linux") AND DISABLE_OPENMP)
        target_compile_options(geogram PRIVATE -fno-openmp)
        target_compile_options(geogram_third_party PRIVATE -fno-openmp)
    endif()

    include(geogram)
endif()



# TBB
if(FLOAT_TETWILD_ENABLE_TBB AND NOT TARGET TBB::tbb)
    FetchContent_Declare(
        tbb
        GIT_REPOSITORY https://github.com/oneapi-src/oneTBB
        GIT_TAG        v2022.2.0
    )

    # Configure TBB build options
    set(TBB_STRICT OFF CACHE BOOL "" FORCE)
    set(TBB_BUILD_STATIC ON CACHE BOOL "" FORCE)
    set(TBB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(TBB_BUILD_TBBMALLOC OFF CACHE BOOL "" FORCE)
    set(TBB_BUILD_TBBMALLOC_PROXY OFF CACHE BOOL "" FORCE)
    set(TBB_TEST OFF CACHE BOOL "" FORCE)
    set(TBB_NO_DATE ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(tbb)
endif()

# C++11 threads
find_package(Threads REQUIRED)

# Json
if(NOT TARGET json)
    FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/jdumas/json
        GIT_TAG        0901d33bf6e7dfe6f70fd9d142c8f5c6695c6c5b
    )
    FetchContent_MakeAvailable(json)
    
    # Create interface target if not provided by the library
    if(NOT TARGET json)
        add_library(json INTERFACE)
        target_include_directories(json SYSTEM
                                 INTERFACE ${json_SOURCE_DIR}/include)
    endif()
endif()

# winding number float_tetwild_download_windingnumber()
# set(windingnumber_SOURCES ${FLOAT_TETWILD_EXTERNAL}/windingnumber/SYS_Math.h
# ${FLOAT_TETWILD_EXTERNAL}/windingnumber/SYS_Types.h
# ${FLOAT_TETWILD_EXTERNAL}/windingnumber/UT_Array.cpp
# ${FLOAT_TETWILD_EXTERNAL}/windingnumber/UT_Array.h
# ${FLOAT_TETWILD_EXTERNAL}/windingnumber/UT_ArrayImpl.h
# ${FLOAT_TETWILD_EXTERNAL}/windingnumber/UT_BVH.h
# ${FLOAT_TETWILD_EXTERNAL}/windingnumber/UT_BVHImpl.h
# ${FLOAT_TETWILD_EXTERNAL}/windingnumber/UT_FixedVector.h
# ${FLOAT_TETWILD_EXTERNAL}/windingnumber/UT_ParallelUtil.h
# ${FLOAT_TETWILD_EXTERNAL}/windingnumber/UT_SmallArray.h
# ${FLOAT_TETWILD_EXTERNAL}/windingnumber/UT_SolidAngle.cpp
# ${FLOAT_TETWILD_EXTERNAL}/windingnumber/UT_SolidAngle.h
# ${FLOAT_TETWILD_EXTERNAL}/windingnumber/VM_SIMD.h
# ${FLOAT_TETWILD_EXTERNAL}/windingnumber/VM_SSEFunc.h )
#
# add_library(fast_winding_number ${windingnumber_SOURCES})
# target_link_libraries(fast_winding_number PRIVATE tbb::tbb)
# target_compile_features(fast_winding_number PRIVATE ${CXX17_FEATURES})
# target_include_directories(fast_winding_number PUBLIC
# "${FLOAT_TETWILD_EXTERNAL}/")

if(FLOAT_TETWILD_WITH_EXACT_ENVELOPE)
    FetchContent_Declare(
        exact_envelope
        GIT_REPOSITORY https://github.com/wangbolun300/fast-envelope
        GIT_TAG        520ee04b6c69a802db31d1fd3a3e6e382d10ef98
    )
    FetchContent_MakeAvailable(exact_envelope)
endif()
