add_subdirectory(libs)
add_subdirectory(source)

add_executable( RideHalSampleApp main.cpp )
target_link_libraries( RideHalSampleApp RideHalSample )
install(TARGETS RideHalSampleApp DESTINATION bin)
