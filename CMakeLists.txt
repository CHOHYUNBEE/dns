cmake_minimum_required(VERSION 3.10.2)
project(dns C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

add_executable(server server.c)
add_executable(client client.c)