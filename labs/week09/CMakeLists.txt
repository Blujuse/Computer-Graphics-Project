cmake_minimum_required(VERSION 3.27.1)

project(Lab9)

add_subdirectory(3rdParty)

include_directories(3rdParty/lodepng)
include_directories(3rdParty/eigen-3.4.0)

add_executable(SphereTracer
    SphereTracer.cpp
    LinAlg.hpp
    Image.hpp
    Light.hpp
    )

target_link_libraries(SphereTracer 
    lodepng
    )
