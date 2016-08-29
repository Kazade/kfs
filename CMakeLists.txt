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

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

SET(KAZTEST_EXECUTABLE ${CMAKE_SOURCE_DIR}/tests/bin/kaztest_gen)

FILE(GLOB_RECURSE TEST_FILES tests/*.h)

ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/tests/main.cpp
    COMMAND ${KAZTEST_EXECUTABLE} --output ${CMAKE_CURRENT_SOURCE_DIR}/tests/main.cpp ${TEST_FILES}
    DEPENDS ${TEST_FILES} ${KAZTEST_EXECUTABLE}
)

ADD_EXECUTABLE(tests ${TEST_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/tests/main.cpp)
target_link_libraries(tests kfs)
