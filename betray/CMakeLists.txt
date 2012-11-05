cmake_minimum_required(VERSION 2.8)
project(betray C)

set( CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR} )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY lib/ )
set( INTERMEDIATE_OUTPUT_PATH obj/ )
set( INCLUDE_CURRENT_DIRECTORY true )

set( LIB_SOURCES b_glut.c b_main.c b_sdl.c b_win32.c betray.h )

#setup the targets
add_library( betray ${LIB_SOURCES} )