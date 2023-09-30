cmake_minimum_required(VERSION 3.23)

project(golos VERSION 0.0.1 LANGUAGES C)

add_subdirectory(deps/raylib/)

set(C_STANDARD 99)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_executable(server src/server.c)
target_include_directories(server PRIVATE include/)
target_link_libraries(server PRIVATE raylib)

add_executable(client src/client.c)
target_include_directories(client PRIVATE include/)
target_link_libraries(client PRIVATE raylib)

