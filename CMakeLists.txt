cmake_minimum_required(VERSION 2.8)
project(kfs)

add_library(kfs SHARED ${CMAKE_SOURCE_DIR}/kfs/kfs.cpp)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")

install(
    DIRECTORY
    kfs/
    DESTINATION
    ${CMAKE_INSTALL_PREFIX}/include/kfs
    FILES_MATCHING REGEX
    ".*\\.(hpp|h)$"
)

INSTALL(TARGETS kfs DESTINATION lib)
