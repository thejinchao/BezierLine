#
#Copyright(C) thecodeway.com
#

cmake_minimum_required (VERSION 3.0)

set(DXUT_CORE_SOURCE_FILES
	DDSTextureLoader.h
	dxerr.h
	DXUT.h
	DXUTDevice11.h
	DXUTmisc.h
	ScreenGrab.h
	WICTextureLoader.h
	DDSTextureLoader.cpp
	dxerr.cpp
	DXUT.cpp
	DXUTDevice11.cpp
	DXUTmisc.cpp
	ScreenGrab.cpp
	WICTextureLoader.cpp
)

add_library(DXUT_Core 
	${DXUT_CORE_SOURCE_FILES} 
)
