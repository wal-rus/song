cmake_minimum_required(VERSION 2.8)
project(enough C)

set( CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR} )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY lib/ )
set( INTERMEDIATE_OUTPUT_PATH c/ )
set( INCLUDE_CURRENT_DIRECTORY true )

set( LIB_SOURCES e_storage_audio.c e_storage_bitmap.c e_storage_curve.c e_storage_geometry.c
	 e_storage_head.c e_storage_material.c e_storage_node.c e_storage_object.c e_storage_text.c
	  ../st/st_types.c e_custom.h e_includes.h e_storage_node.h e_storage_object.h e_types.h enough.h )


#setup the targets
add_library( enough ${LIB_SOURCES} )
