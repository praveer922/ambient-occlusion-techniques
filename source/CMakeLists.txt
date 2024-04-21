cmake_minimum_required(VERSION 3.13)
project(AOdemo)


set(SOURCES main.cpp includes/lodepng.cpp)
add_executable(AOdemo ${SOURCES})

target_link_libraries(AOdemo GL)
target_link_libraries(AOdemo glut)
target_link_libraries(AOdemo GLEW)
target_link_libraries(AOdemo SDL2)