
add_subdirectory(BufferManager)
add_subdirectory(SharedRing)
add_subdirectory(TinyViz)

if( ENABLE_DEMUXER )
    add_subdirectory(VidcDemuxer)
endif()

if ( ENABLE_C2D )
  add_subdirectory(C2D)
endif()

add_subdirectory(GL2DFlex)

if ( ENABLE_FADAS )
  add_subdirectory(PostCenterPoint)
endif()
