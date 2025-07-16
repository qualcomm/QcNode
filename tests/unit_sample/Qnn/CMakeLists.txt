
if( NOT TARGET QCNodeQNN )
  message(STATUS "skip building of qnn_test")
else()
  add_executable( QCNodeQnnTest qnn_test.cpp )
  target_include_directories( QCNodeQnnTest PUBLIC ${HEADERS_DIR} )
  target_link_libraries( QCNodeQnnTest gtest QCNodeQNN QCNodeMemory )
  install(TARGETS QCNodeQnnTest DESTINATION bin)
endif()
