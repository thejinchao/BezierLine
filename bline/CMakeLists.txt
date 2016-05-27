#
#Copyright(C) thecodeway.com
#

cmake_minimum_required (VERSION 3.0)

set(BLINE_SOURCE_FILES
	bl_main.cpp
	bl_line.h
	bl_line.cpp
	bl_helper.h
	bl_helper.cpp
)

include_directories(
	../DXUT/Core
	../DXUT/Optional
)

add_executable(bline WIN32
	${BLINE_SOURCE_FILES}
)

target_link_libraries(bline
	DXUT_Core
	DXUT_Optional
	d3dcompiler.lib
	usp10.lib
	dxguid.lib
	winmm.lib
	comctl32.lib	
)
