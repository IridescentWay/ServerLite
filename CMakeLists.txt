cmake_minimum_required(VERSION 3.25)

project(ServerLite)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -std=c++14")
set(source_dir "${PROJECT_SOURCE_DIR}/src/")

file(GLOB_RECURSE source_files "${source_dir}/*.*")

add_executable(ServerLite ${source_files})

target_link_libraries(ServerLite pthread mysqlclient)