cmake_minimum_required(VERSION 2.8)
project(verse C)

set( CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR} )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY bin/ )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY lib/ )
set( INTERMEDIATE_OUTPUT_PATH obj/ )
set( INCLUDE_CURRENT_DIRECTORY true )

set( LIB_SOURCES v_bignum.c v_cmd_buf.c v_cmd_def_a.c v_cmd_def_b.c v_cmd_def_c.c v_cmd_def_g.c v_cmd_def_m.c v_cmd_def_o.c v_cmd_def_s.c v_cmd_def_t.c v_cmd_gen.c v_connect.c v_connection.c v_encryption.c v_func_storage.c v_gen_pack_a_node.c v_gen_pack_b_node.c v_gen_pack_c_node.c v_gen_pack_g_node.c v_gen_pack_init.c v_gen_pack_m_node.c v_gen_pack_o_node.c v_gen_pack_s_node.c v_gen_pack_t_node.c v_man_pack_node.c v_network.c v_network_in_que.c v_network_out_que.c v_pack.c v_pack_method.c v_prime.c v_randgen.c v_util.c )

set( EXE_SOURCES  vs_connection.c vs_main.c vs_master.c  vs_node_audio.c vs_node_bitmap.c vs_node_curve.c vs_node_geometry.c vs_node_head.c vs_node_material.c vs_node_object.c vs_node_particle.c vs_node_storage.c vs_node_text.c )

#setup the targets
add_library(verselibrary ${LIB_SOURCES} )
set_target_properties( verselibrary PROPERTIES OUTPUT_NAME verse )

add_executable( verseserver ${EXE_SOURCES} )
target_link_libraries(verseserver verse wsock32.lib)
set_target_properties( verseserver PROPERTIES OUTPUT_NAME verse )
