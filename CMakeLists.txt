cmake_minimum_required(VERSION 2.8.3)
project(bing)

set(CMAKE_CXX_FLAGS "-std=c++11 -g -Wall -Wextra")

# Qt
find_package(Qt5Core)
find_package(Qt5Concurrent)
find_package(Qt5Gui)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSOUP libsoup-2.4)

include_directories(
  include
  ${LIBSOUP_INCLUDE_DIRS}
)

qt5_wrap_cpp(
  all_moc
  speech.hpp
  qnamaker.hpp
  customvision.hpp
)

# Library
add_library(
  bing
  SHARED
  speech.cpp
  qnamaker.cpp
  customvision.cpp
  ${all_moc}
)
target_link_libraries(
  bing
  Qt5::Core
  Qt5::Gui
)

# Bing Speech (Recognition)
add_executable(
  bingspeech_recognition_example
  examples/bingspeech_recognition_example.cpp
  ${all_moc}
)
target_link_libraries(
  bingspeech_recognition_example
  bing
  ${LIBSOUP_LIBRARIES}
  Qt5::Core
)

# Bing Speech (Synthesis)
add_executable(
  bingspeech_synthesis_example
  examples/bingspeech_synthesis_example.cpp
  ${all_moc}
)
target_link_libraries(
  bingspeech_synthesis_example
  bing
  ${LIBSOUP_LIBRARIES}
  Qt5::Core
)

# Bing QnaMaker
add_executable(
  bingqnamaker_example
  examples/bingqnamaker_example.cpp
  ${all_moc}
)
target_link_libraries(
  bingqnamaker_example
  bing
  ${LIBSOUP_LIBRARIES}
  Qt5::Core
  Qt5::Concurrent
)

# Bing Custom Vision
add_executable(
  bingcustomvision_example
  examples/bingcustomvision_example.cpp
  ${all_moc}
)
target_link_libraries(
  bingcustomvision_example
  bing
  ${LIBSOUP_LIBRARIES}
  Qt5::Core
  Qt5::Gui
)

# Generate pkg-config
set(DEST_DIR "${CMAKE_INSTALL_PREFIX}")
set(PRIVATE_LIBS "-lbing")
CONFIGURE_FILE("bing.pc.in" "bing.pc" @ONLY)

# Install
install(TARGETS bing DESTINATION lib)
install(TARGETS bingspeech_recognition_example DESTINATION bin)
install(TARGETS bingspeech_synthesis_example DESTINATION bin)
install(
    FILES "bing.hpp" "qnamaker.hpp" "speech.hpp" "customvision.hpp" "exception.hpp" DESTINATION include/bing
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
install(
    FILES "bing.pc" DESTINATION lib/pkgconfig
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
