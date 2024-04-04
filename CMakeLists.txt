cmake_minimum_required(VERSION 3.19)
project( bm CXX )

# Libraries
add_subdirectory( lib/concurrentqueue )
add_subdirectory( lib/readerwriterqueue )
add_subdirectory( lib/bm_core )

# Applications
add_subdirectory( app/example_app )
