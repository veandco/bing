cmake_minimum_required(VERSION 2.8.3)
project(libbing)

set(CMAKE_CXX_FLAGS "-std=c++11")

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSOUP libsoup-2.4)
pkg_check_modules(JSONCPP jsoncpp)

include_directories(
  include
  ${LIBSOUP_INCLUDE_DIRS}
  ${JSONCPP_INCLUDE_DIRS}
)

# Library
add_library(
  bing
  SHARED
  src/bing.cpp
  src/bing/speech.cpp
)

# Bing Speech (Recognition)
add_executable(
  bingspeech_recognition_example
  src/bingspeech_recognition_example.cpp
)
target_link_libraries(
  bingspeech_recognition_example
  bing
  ${LIBSOUP_LIBRARIES}
  ${JSONCPP_LIBRARIES}
)

# Install
install(TARGETS bingspeech_recognition_example DESTINATION bin)
install(TARGETS bing DESTINATION lib)
