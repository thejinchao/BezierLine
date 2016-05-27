#
#Copyright(C) thecodeway.com
#

cmake_minimum_required (VERSION 3.0)

set(DXUT_OPTIONAL_SOURCE_FILES
	DXUTcamera.h
	DXUTgui.h
	DXUTguiIME.h
	DXUTLockFreePipe.h
	DXUTres.h
	DXUTsettingsdlg.h
	ImeUi.h
	SDKmesh.h
	SDKmisc.h
	DXUTcamera.cpp
	DXUTgui.cpp
	DXUTguiIME.cpp
	DXUTres.cpp
	DXUTsettingsdlg.cpp
	ImeUi.cpp
	SDKmesh.cpp
	SDKmisc.cpp
)

include_directories(
	../Core
)

add_library(DXUT_Optional 
	${DXUT_OPTIONAL_SOURCE_FILES} 
)
