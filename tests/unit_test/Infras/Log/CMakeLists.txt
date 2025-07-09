
add_executable( gtest_Logger gtest_Logger.cpp )
target_link_libraries( gtest_Logger gtest QCNodeCommon )
install(TARGETS gtest_Logger DESTINATION bin)
