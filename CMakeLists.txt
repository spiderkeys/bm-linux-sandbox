cmake_minimum_required(VERSION 3.19)
project( bm CXX )

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

#############################
# Options

# Define an option to include the subdirectory
option(BUILD_BMGUI "Build the bmgui application" OFF)

#############################
# Common Dependencies

include(FetchContent)

# Fetch Boost
set(BOOST_INCLUDE_LIBRARIES system program_options asio)
set(BOOST_ENABLE_CMAKE ON)
FetchContent_Declare(
  Boost
  URL https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.7z
  USES_TERMINAL_DOWNLOAD TRUE
  DOWNLOAD_NO_EXTRACT FALSE
)
FetchContent_MakeAvailable(Boost)

# Fetch spdlog
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.9.2
)
FetchContent_MakeAvailable(spdlog)

#############################

# Libraries
add_subdirectory( lib/bm_core )

# Applications
if(BUILD_BMGUI)
  add_subdirectory( app/bmgui )
endif()
