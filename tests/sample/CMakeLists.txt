add_subdirectory(libs)
add_subdirectory(source)

add_executable( QCNodeSampleApp main.cpp )
target_link_libraries( QCNodeSampleApp QCNodeSample )
install(TARGETS QCNodeSampleApp DESTINATION bin)
