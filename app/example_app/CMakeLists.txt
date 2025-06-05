cmake_minimum_required(VERSION 3.19)
project(example_app CXX)

# ==============================================
# Dependencies

include(FetchContent)

FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG v5.0.0
)

FetchContent_GetProperties(ftxui)
# if(NOT ftxui_POPULATED)
  FetchContent_Populate(ftxui)
  add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
# endif()

# ==============================================
# Targets

add_executable( ${PROJECT_NAME}
  src/main.cpp
  src/example_app.cpp

  bm_core/src/neighbor_table.cpp
  bm_core/src/network_device.cpp
  bm_core/src/network_interface.cpp
  bm_core/src/node.cpp
)

target_link_libraries( ${PROJECT_NAME}
PRIVATE
  Boost::program_options
  Boost::asio
  ftxui::screen 
  ftxui::dom 
  ftxui::component
  spdlog::spdlog
)

target_include_directories(${PROJECT_NAME}
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:include>
  bm_core/include
)

target_compile_options(${PROJECT_NAME}
PUBLIC
  -Wall 
  -Wextra 
  -Wpedantic
)

target_compile_features(${PROJECT_NAME}
PUBLIC
  cxx_std_17
)

# ==============================================
# Install
install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION bin
)