# TODO: Add defines for solaris and hp-ux.
# TODO: Maybe not depend on cmake 3 but the project command is nice in cmake 3.

cmake_minimum_required(VERSION 3.0.0)

project(qore-sqlite3-module VERSION 1.0.1)

include(CheckCXXCompilerFlag)

find_package(Qore REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if(CMAKE_USE_PTHREADS_INIT)
message(STATUS "Found POSIX Threads: TRUE")
else(CMAKE_USE_PTHREADS_INIT)
message(STATUS "Found POSIX Threads: FALSE")
message(FATAL_ERROR "POSIX threads does not seem to be supported on this platform, aborting")
endif()

include_directories(${SQLITE3_INCLUDE_DIRS})

check_cxx_compiler_flag(-fvisibility=hidden HAVE_GCC_VISIBILITY)

if(${CMAKE_SYSTEM_NAME} EQUAL "Linux")
add_definitions(-D_GNU_SOURCE)
endif()

configure_file(${CMAKE_SOURCE_DIR}/cmake/config.h.in config.h)

set(CPP_SRC
    src/sqlite3connection.cc
    src/sqlite3executor.cc
    src/sqlite3module.cc
)

set(module_name "sqlite3")

add_library(${module_name} SHARED ${CPP_SRC})

qore_binary_module(${module_name} ${PROJECT_VERSION} ${SQLITE3_LDFLAGS} Threads::Threads)

qore_dist(${PROJECT_VERSION})

qore_config_info()
