
cmake_minimum_required(VERSION 3.15.0)

project(ridehal LANGUAGES C CXX)

option(ENABLE_GCOV "Enable GCC Code Coverage" OFF)
option(ENABLE_DEMUXER "Enable Demuxer building" OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "-g")

if( ENABLE_GCOV )
  set(CMAKE_CXX_FLAGS_RELEASE "-O0 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O0 -g -DNDEBUG")
  add_compile_options(-ftest-coverage -fprofile-arcs)
  add_link_options(-ftest-coverage -fprofile-arcs)
else()
  set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmacro-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=/ridehal")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmacro-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=/ridehal")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(scripts)
add_subdirectory(source)
add_subdirectory(tests)
