project(teste CXX)
cmake_minimum_required(VERSION 3.16)

file(GLOB SOURCES
    "*.cpp")


add_executable(teste ${SOURCES})
target_compile_options(teste PUBLIC -std=c++20)
target_compile_definitions(teste PUBLIC -DMAINSRCDIR="${CMAKE_SOURCE_DIR}")
