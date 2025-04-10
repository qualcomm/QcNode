
set( GTEST_RIDEHAL_SOURCES gtest_RideHal.cpp )
set( GTEST_RIDEHAL_INCLUDES ${PROJECT_SOURCE_DIR}/include )
set( GTEST_RIDEHAL_LIBS gtest RideHal RideHalTestUtils RideHalSample)

add_subdirectory(buffer)
add_subdirectory(components)
add_subdirectory(logger)

add_executable( gtest_RideHal ${GTEST_RIDEHAL_SOURCES} )
target_link_libraries( gtest_RideHal ${GTEST_RIDEHAL_LIBS} )
target_compile_definitions( gtest_RideHal PUBLIC GTEST_RIDEHAL )
target_include_directories( gtest_RideHal PUBLIC ${GTEST_RIDEHAL_INCLUDES} )

install(TARGETS gtest_RideHal DESTINATION bin)
