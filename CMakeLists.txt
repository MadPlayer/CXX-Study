cmake_minimum_required(VERSION 4.3)
project(CXX_Practice VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


find_package(Boost 1.84 REQUIRED)

add_subdirectory(Describe)
add_subdirectory(Asio)
