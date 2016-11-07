cmake_minimum_required(VERSION 3.0)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

include(FindPkgConfig)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Multimedia REQUIRED)
pkg_check_modules(SPEEXDSP speexdsp REQUIRED)

include_directories(${SPEEXDSP_INCLUDE_DIRS})
link_directories(${SPEEXDSP_LIBRARY_DIRS})

set(dessiner_un_son_SOURCES
  sources/dot-editor-widget.cc
  sources/wave-generator.cc
  sources/wave-io.cc
  sources/wave-io-dialog.cc
  sources/math-dsp.cc
  sources/main.cc)
add_executable(dessiner-un-son ${dessiner_un_son_SOURCES})
set_property(TARGET dessiner-un-son PROPERTY CXX_STANDARD 14)
target_link_libraries(dessiner-un-son Qt5::Widgets)
target_link_libraries(dessiner-un-son Qt5::Multimedia)
target_link_libraries(dessiner-un-son ${SPEEXDSP_LIBRARIES})
