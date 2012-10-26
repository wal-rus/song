cmake_minimum_required(VERSION 2.8)
project(deceive C)

set( CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR} )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY lib/ )
set( INTERMEDIATE_OUTPUT_PATH c/ )
set( INCLUDE_CURRENT_DIRECTORY true )

set( LIB_SOURCES d_main.c d_master.c )

#setup the targets
add_library( deceive ${LIB_SOURCES} )

