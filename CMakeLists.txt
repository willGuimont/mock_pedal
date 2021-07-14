cmake_minimum_required(VERSION 3.19)
project(mock_pedal C)

set(CMAKE_C_STANDARD 99)

add_executable(mock_pedal main.c)
target_link_libraries(mock_pedal X11 Xi xdo)

add_executable(mock_pedal2 main2.c)
target_link_libraries(mock_pedal2 X11 Xtst)
