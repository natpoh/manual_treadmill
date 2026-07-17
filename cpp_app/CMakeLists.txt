cmake_minimum_required(VERSION 3.14)
project(VRTreadmill CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# ImGui
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

# ImPlot
FetchContent_Declare(
  implot
  GIT_REPOSITORY https://github.com/epezent/implot.git
  GIT_TAG master
)
FetchContent_MakeAvailable(implot)

# ViGEmClient
FetchContent_Declare(
  vigemclient
  GIT_REPOSITORY https://github.com/nefarius/ViGEmClient.git
  GIT_TAG master
)
FetchContent_MakeAvailable(vigemclient)

add_executable(VRTreadmill WIN32
  main.cpp
  Gui.cpp
  SensorReader.cpp
  GamepadOutput.cpp
  MovementTracker.cpp
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
  ${implot_SOURCE_DIR}/implot.cpp
  ${implot_SOURCE_DIR}/implot_items.cpp
)

target_include_directories(VRTreadmill PRIVATE
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
  ${implot_SOURCE_DIR}
  ${vigemclient_SOURCE_DIR}/include
)

target_compile_options(VRTreadmill PRIVATE /utf-8)

target_link_libraries(VRTreadmill PRIVATE
  d3d11
  d3dcompiler
  dwmapi
  ViGEmClient
  setupapi
)

# Copy dlls to output directory (if shared)
add_custom_command(TARGET VRTreadmill POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:ViGEmClient>"
    $<TARGET_FILE_DIR:VRTreadmill>
)
