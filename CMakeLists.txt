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

# Bing Speech
add_executable(
  bingspeech_example
  src/bingspeech_example.cpp
  src/bing.cpp
  src/bing/speech.cpp
)
target_link_libraries(
  bingspeech_example
  ${LIBSOUP_LIBRARIES}
  ${JSONCPP_LIBRARIES}
)
