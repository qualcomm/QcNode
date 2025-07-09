
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

if( NOT TARGET QCNode )
  find_package(QCNode)
  if( NOT TARGET QCNode )
    message(FATAL_ERROR "QCNode SDK not found")
  endif()
endif()

target_link_libraries(SharedRing 
    QCNode
)

target_link_directories(SharedRing PUBLIC ${CMAKE_PREFIX_PATH}/lib)

target_include_directories(SharedRing PUBLIC ${HEADERS_DIR})

set_property( TARGET SharedRing PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET SharedRing PROPERTY POSITION_INDEPENDENT_CODE ON )

add_executable( QCNodeSharedRingInspect utils/SharedRingInspect.cpp )
target_link_libraries( QCNodeSharedRingInspect SharedRing )
if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
  target_link_libraries( QCNodeSharedRingInspect rt )
  target_link_options( QCNodeSharedRingInspect PUBLIC -pthread )
endif()
install(TARGETS QCNodeSharedRingInspect DESTINATION bin)
