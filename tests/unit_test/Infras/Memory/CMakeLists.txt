
if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64" )
    add_executable( gtest_Memory 
        gtest_Memory.cpp 
        gtest_QCMemoryManager.cpp 
        gtest_QCHEAPMemoryAllocator.cpp 
        gtest_QCDMABUFFMemoryAllocator.cpp 
        gtest_QCMemoryPool.cpp
        gtest_QCMemoryUtilsBase.cpp)
else()
    add_executable( gtest_Memory 
        gtest_Memory.cpp 
        gtest_QCMemoryManager.cpp 
        gtest_QCHEAPMemoryAllocator.cpp
        gtest_QCPMEMMemoryAllocator.cpp 
        gtest_QCMemoryPool.cpp
        gtest_QCMemoryUtilsBase.cpp)
endif()

target_link_libraries( gtest_Memory gtest QCNodeCommon QCNodeMemory )
install(TARGETS gtest_Memory DESTINATION bin)
