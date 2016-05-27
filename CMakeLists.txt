#
#Copyright(C) thecodeway.com
#

cmake_minimum_required (VERSION 3.0)

project(bline)


########
#global defines
########
add_definitions(-DUNICODE -D_UNICODE_)


########
#sub dictionary
########
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
add_subdirectory(DXUT)
add_subdirectory(bline)

