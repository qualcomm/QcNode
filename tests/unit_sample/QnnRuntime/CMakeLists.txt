
if( NOT TARGET RideHalQnnRuntime )
  message(STATUS "skip building of qnn_test")
else()
  add_executable( RideHalQnnTest qnn_test.cpp )
  target_include_directories( RideHalQnnTest PUBLIC ${HEADERS_DIR} )
  target_link_libraries( RideHalQnnTest gtest RideHalComponentCommon RideHalQnnRuntime )
  install(TARGETS RideHalQnnTest DESTINATION bin)
endif()
