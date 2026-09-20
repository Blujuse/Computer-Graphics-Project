cmake_minimum_required(VERSION 3.27.1)

project(Lab3)

add_subdirectory(3rdParty)

include_directories(3rdParty/lodepng)

add_executable(Task1
    Task1.cpp
    Vector3.hpp
    Vector2.hpp)


target_link_libraries(Task1 
    lodepng
    )
