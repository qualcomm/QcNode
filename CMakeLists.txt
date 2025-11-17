
cmake_minimum_required(VERSION 3.15.0)

project(qcnode LANGUAGES C CXX)

# valid options: 8295 8620 8650 8775 8797, override it from the command as below:
#  cmake -DQC_TARGET_SOC=8797 ...
set(QC_TARGET_SOC "8797" CACHE STRING "QCNode Target SOC")

if(QC_TARGET_SOC STREQUAL "8295")
    message(STATUS "Target SOC is 8295")
    add_compile_definitions(QC_TARGET_SOC=8295)
elseif(QC_TARGET_SOC STREQUAL "8620")
    message(STATUS "Target SOC is 8620")
    add_compile_definitions(QC_TARGET_SOC=8620)
elseif(QC_TARGET_SOC STREQUAL "8650")
    message(STATUS "Target SOC is 8650")
    add_compile_definitions(QC_TARGET_SOC=8650)
elseif(QC_TARGET_SOC STREQUAL "8775")
    message(STATUS "Target SOC is 8775")
    add_compile_definitions(QC_TARGET_SOC=8775)
elseif(QC_TARGET_SOC STREQUAL "8797")
    message(STATUS "Target SOC is 8797")
    add_compile_definitions(QC_TARGET_SOC=8797)
else()
    message(FATAL_ERROR "Unknown target SOC: ${QC_TARGET_SOC}")
endif()

option(ENABLE_GCOV "Enable GCC Code Coverage" OFF)
option(ENABLE_CTC "Enable CTC Code Coverage" OFF)
option(ENABLE_DEMUXER "Enable Demuxer building" OFF)
option(ENABLE_C2D "Enable C2D building" OFF)
option(ENABLE_EVA "Enable EVA building" OFF)
option(ENABLE_FADAS "Enable FADAS building" OFF)
option(ENABLE_RSM_V2 "Enable RSM_V2 building" OFF)
option(ENABLE_RADAR "Enable Radar building" OFF)
option(ENABLE_C2C "Enable C2C building" OFF)
option(ENABLE_TRACE "Enable Trace building" OFF)
option(ENABLE_EVA_AUTO "Enable EVA AUTO building" OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")

if( ENABLE_GCOV )
  set(CMAKE_CXX_FLAGS_RELEASE "-O0 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O0 -g -DNDEBUG")
  add_compile_options(-ftest-coverage -fprofile-arcs)
  add_link_options(-ftest-coverage -fprofile-arcs)
elseif( ENABLE_CTC )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=default")
  link_libraries(ctc)
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
  set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
endif()


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmacro-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=/qcnode")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmacro-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=/qcnode")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(scripts)
add_subdirectory(source)
add_subdirectory(tests)
