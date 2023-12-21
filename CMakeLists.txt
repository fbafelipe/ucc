cmake_minimum_required(VERSION 2.8.12)

PROJECT(UNNAMED_C_COMPILER)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/grammar/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../ucc-vm/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libparser/include)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../ucc-vm/build)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libparser/build)

FILE(GLOB_RECURSE sources ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
FILE(GLOB_RECURSE generated_sources ${CMAKE_CURRENT_SOURCE_DIR}/grammar/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/grammar/src/*.h)

ADD_EXECUTABLE(ucc ${sources} ${generated_sources})
target_link_libraries(ucc ucc-vm parser)
