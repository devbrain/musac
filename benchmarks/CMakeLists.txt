cmake_minimum_required(VERSION 3.20)

# Option to enable/disable benchmarks
option(MUSAC_BUILD_BENCHMARKS "Build benchmarks" ON)

if(NOT MUSAC_BUILD_BENCHMARKS)
    return()
endif()

# Try to find nanobench or fetch it
find_package(nanobench QUIET)

if(NOT nanobench_FOUND)
    message(STATUS "nanobench not found, fetching from GitHub...")
    
    include(FetchContent)
    set(FETCHCONTENT_QUIET OFF)
    
    # Allow FetchContent_Populate to be used
    if(POLICY CMP0169)
        cmake_policy(SET CMP0169 OLD)
    endif()
    
    FetchContent_Declare(
        nanobench
        GIT_REPOSITORY https://github.com/martinus/nanobench.git
        GIT_TAG v4.3.11
        GIT_PROGRESS TRUE
    )
    
    FetchContent_MakeAvailable(nanobench)
endif()

# Create benchmark executable
add_executable(musac_benchmark
    # Main benchmark runner
    benchmark_main.cc
    
    # Core benchmarks
    core/benchmark_stream_operations.cc
    core/benchmark_mixer_operations.cc
    core/benchmark_lock_contention.cc
    
    # Threading benchmarks
    threading/benchmark_phase1_destruction.cc
    threading/benchmark_phase2_locking.cc
)

# Include directories
target_include_directories(musac_benchmark
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}
)

# Link libraries
target_link_libraries(musac_benchmark
    PRIVATE
        nanobench::nanobench
        musac
        musac_sdk
        musac_codecs
        thirdparty::failsafe
)

# Compile options
target_compile_options(musac_benchmark PRIVATE ${MUSAC_WARNING_FLAGS})

# Add custom target for running benchmarks
add_custom_target(run_benchmarks
    COMMAND musac_benchmark
    DEPENDS musac_benchmark
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Running benchmarks..."
)