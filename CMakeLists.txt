cmake_minimum_required(VERSION 2.8)
project(song C CXX)
include_directories(st)
include_directories(betray)
include_directories(enough)
include_directories(verse)
include_directories(seduce)
include_directories(deceive)

set(TARGETS 
		betray
		#co_on
		deceive 
		enough 
		#loq_airou 
		persuade 
		#quel_solaar 
		#resources 
		seduce 
		#uv
)

foreach( loop_var ${TARGETS} )
	add_subdirectory( ${loop_var} )
endforeach()

set_target_properties( PROPERTIES COMPILE_DEFINITIONS _CRT_SECURE_NO_DEPRECATE)