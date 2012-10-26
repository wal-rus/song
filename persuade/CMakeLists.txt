cmake_minimum_required(VERSION 2.8)
project(persuade C)

set( CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR} )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY lib/ )
set( INTERMEDIATE_OUTPUT_PATH c/ )
set( INCLUDE_CURRENT_DIRECTORY true )

set( LIB_SOURCES p_extension.c p_flare.c p_geometry.c p_impostor.c p_noise.c p_object_environment.c p_object_handle.c p_object_light.c p_object_param.c p_object_render.c p_render_to_texture.c p_sds_array.c p_sds_geo_clean.c p_sds_geo_divide.c p_sds_obj.c p_sds_obj_anim.c p_sds_obj_displace.c p_sds_obj_edge_normal.c p_sds_obj_param.c p_sds_obj_sort.c p_sds_obj_tess.c p_sds_table.c p_sds_table_debug.c p_sds_table_edge_sort.c p_sds_table_normals.c p_sds_table_split.c p_shader_extension.c p_shader_gl_one_fall_back.c p_shader_ramp_texture.c p_shader_writer.c p_status_print.c p_task.c p_texture.c )

#setup the targets
add_library( persuade ${LIB_SOURCES} )

