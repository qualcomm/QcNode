
add_subdirectory(SharedRing)
add_subdirectory(TinyViz)

if( ENABLE_DEMUXER )
    add_subdirectory(VidcDemuxer)
endif()
