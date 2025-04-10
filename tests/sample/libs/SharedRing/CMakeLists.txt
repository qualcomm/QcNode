
set( HEADERS_DIR
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

add_library(SharedRing STATIC
    source/SharedRing.cpp
    source/SharedMemory.cpp
    source/SharedSemaphore.cpp
    source/SpinLock.cpp
    source/SharedPublisher.cpp
    source/SharedSubscriber.cpp
)

target_include_directories(SharedRing PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_PREFIX_PATH}/include
)

if( NOT TARGET RideHal )
  find_package(RideHal)
  if( NOT TARGET RideHal )
    message(FATAL_ERROR "RideHal SDK not found")
  endif()
endif()

target_link_libraries(SharedRing 
    RideHal
)

target_link_directories(SharedRing PUBLIC ${CMAKE_PREFIX_PATH}/lib)

target_include_directories(SharedRing PUBLIC ${HEADERS_DIR})

set_property( TARGET SharedRing PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET SharedRing PROPERTY POSITION_INDEPENDENT_CODE ON )

add_executable( RideHalSharedRingInspect utils/SharedRingInspect.cpp )
target_link_libraries( RideHalSharedRingInspect SharedRing )
if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
  target_link_libraries( RideHalSharedRingInspect rt )
  target_link_options( RideHalSharedRingInspect PUBLIC -pthread )
endif()
install(TARGETS RideHalSharedRingInspect DESTINATION bin)
