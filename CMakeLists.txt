cmake_minimum_required( VERSION 3.10 )
project( "MuTra (Music Trainer)" VERSION 0.0.1 DESCRIPTION "Music (piano ATM) training library and application" LANGUAGES CXX )
include_directories(  "${PROJECT_SOURCE_DIR}/lib" )
add_subdirectory( lib )
add_subdirectory( cli )
add_subdirectory( app )
