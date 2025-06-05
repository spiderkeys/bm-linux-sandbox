cmake_minimum_required(VERSION 3.19)
project(bm_core CXX)

# ==============================================
# Targets

add_library( ${PROJECT_NAME} 
    "src/network_device.cpp"
    "src/network_interface.cpp"
    "src/node.cpp"  
)

target_include_directories( ${PROJECT_NAME}  
PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_options( ${PROJECT_NAME}  
PUBLIC
    -Wall 
    -Wextra 
    -Wpedantic
)

target_compile_features( ${PROJECT_NAME} 
PUBLIC
    cxx_std_17
)

target_link_libraries( ${PROJECT_NAME} 
PUBLIC
    spdlog::spdlog
    Boost::system
)

# ==============================================
# Install

install(
  DIRECTORY include/
  DESTINATION include
)
install(
  TARGETS ${PROJECT_NAME} 
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)