add_executable( gtest_Memory gtest_Memory.cpp )
target_link_libraries( gtest_Memory gtest QCNodeCommon QCNodeMemory )
install(TARGETS gtest_Memory DESTINATION bin)
