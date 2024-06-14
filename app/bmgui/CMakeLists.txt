cmake_minimum_required(VERSION 3.19)
project(bmgui CXX)

# ==============================================
# Dependencies

find_package(OpenGL REQUIRED)

include(FetchContent)

# Fetch libcap
FetchContent_Declare(
  libcap
  GIT_REPOSITORY https://git.kernel.org/pub/scm/libs/libcap/libcap.git
  GIT_TAG libcap-2.70
)
FetchContent_MakeAvailable(libcap)

# Fetch GLFW
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG latest
)
FetchContent_MakeAvailable(glfw)

# Fetch GLAD (for OpenGL function loading)
FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG master
)
FetchContent_MakeAvailable(glad)

# Fetch Dear ImGui
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG master
)
FetchContent_MakeAvailable(imgui)

# Create a target for Dear ImGui
add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)

# Fetch imgui_toggle
FetchContent_Declare(
  imgui_toggle
  GIT_REPOSITORY https://github.com/cmdwtf/imgui_toggle.git
  GIT_TAG main
)
FetchContent_MakeAvailable(imgui_toggle)

# Create a target for imgui_toggle
add_library(imgui_toggle STATIC
  ${imgui_toggle_SOURCE_DIR}/imgui_toggle_palette.cpp
  ${imgui_toggle_SOURCE_DIR}/imgui_toggle_presets.cpp
  ${imgui_toggle_SOURCE_DIR}/imgui_toggle_renderer.cpp
  ${imgui_toggle_SOURCE_DIR}/imgui_toggle.cpp
)
target_include_directories(imgui_toggle PUBLIC ${imgui_toggle_SOURCE_DIR})
target_link_libraries(imgui_toggle PUBLIC imgui)

# ==============================================
# Targets

add_executable( ${PROJECT_NAME}
  src/main.cpp
  src/application.cpp
  src/util.cpp
  src/backend/node_instance.cpp
  src/backend/raw_socket_device.cpp
)

target_link_libraries( ${PROJECT_NAME}
PRIVATE
  bm_core

  Boost::program_options
  Boost::asio

  glfw 
  glad 
  imgui
  imgui_toggle
  OpenGL::GL

  cap
)

target_include_directories(${PROJECT_NAME}
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:include>
  ${glfw_SOURCE_DIR}/include 
  ${glad_SOURCE_DIR}/include
  ${libcap_SOURCE_DIR}/libcap/include
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