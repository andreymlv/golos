cmake_minimum_required(VERSION 3.23)

project(golos VERSION 0.0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_executable(server src/server.c)
target_include_directories(server PUBLIC include/)
target_link_libraries(server pthread m dl raylib)

add_executable(client src/client.c)
target_include_directories(client PUBLIC include/)
target_link_libraries(client pthread m dl raylib)

if(MSVC)
  target_compile_options(server PRIVATE /W4 /WX)
  target_compile_options(client PRIVATE /W4 /WX)
else()
  target_compile_options(server PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(client PRIVATE -Wall -Wextra -Wpedantic)
endif()

