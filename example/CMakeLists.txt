cmake_minimum_required(VERSION 3.20)

project(Example)

set(CMAKE_CXX_STANDARD 20)

add_executable(Example main.cpp ../src/ArgParser.cpp)