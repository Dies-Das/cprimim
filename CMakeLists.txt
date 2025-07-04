cmake_minimum_required(VERSION 3.22)
project(cprimim C)


# Use C11

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)

set(CMAKE_C_FLAGS_RELEASE   "-march=native -O3 -DNDEBUG")

set(CMAKE_C_FLAGS_RELWITHDEBINFO  "-march=native -O0 -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS_PROFILE "-O2 -g -pg")
find_package(OpenMP REQUIRED)

# Put ALL executables into <project_root>/bin

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

foreach(cfg IN ITEMS Debug Release RelWithDebInfo MinSizeRel)

  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfg} "${CMAKE_SOURCE_DIR}/bin")
endforeach()


# Add subprojects

add_subdirectory(src/processing)

add_subdirectory(src/cli)

