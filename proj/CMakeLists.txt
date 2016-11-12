cmake_minimum_required( VERSION 3.0 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

project( agame )

get_filename_component( CINDER_PATH "/home/aaron/Tools/Cinder" ABSOLUTE )
get_filename_component( APP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../" ABSOLUTE )
get_filename_component( INCLUDE_PATH "${APP_PATH}/include" ABSOLUTE )

include_directories( ${INCLUDE_PATH} )

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

ci_make_app(
  SOURCES     ${APP_PATH}/src/agame.cc ${INCLUDE_PATH}/resources.h
	CINDER_PATH ${CINDER_PATH}
)
