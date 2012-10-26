cmake_minimum_required(VERSION 2.8)
project(seduce C)

set( CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR} )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY lib/ )
set( INTERMEDIATE_OUTPUT_PATH c/ )
set( INCLUDE_CURRENT_DIRECTORY true )

set( LIB_SOURCES s_background.c s_draw.c s_editor.c s_line_font.c s_main.c s_popup.c s_settings.c s_settings_window.c s_text.c )

#setup the targets
add_library( seduce ${LIB_SOURCES} )
