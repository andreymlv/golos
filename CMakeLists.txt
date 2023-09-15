cmake_minimum_required(VERSION 3.23)

project(golos VERSION 0.0.1 LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)


add_executable(server src/server.c)
target_include_directories(server PUBLIC include/)

add_executable(client src/client.c)
target_link_libraries(client PUBLIC pthread m dl)
target_include_directories(client PUBLIC include/)
