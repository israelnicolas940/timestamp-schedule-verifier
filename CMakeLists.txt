cmake_minimum_required(VERSION 3.0.0)
project(Timestamp-Scheduler CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Copy compile_commands.json to project root after generation
add_custom_target(copy-compile-commands ALL
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  "${CMAKE_BINARY_DIR}/compile_commands.json"
  "${CMAKE_SOURCE_DIR}/compile_commands.json"
)
add_executable(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/main.cpp)
