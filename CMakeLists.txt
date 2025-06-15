cmake_minimum_required(VERSION 3.25.1)

set(CMAKE_BUILD_TYPE Debug)

project(recipe-machine-2)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(recipe-machine-2 main.c)

set_target_properties(recipe-machine-2 PROPERTIES LINKER_LANGUAGE C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED gtk+-3.0)

target_include_directories(recipe-machine-2 PRIVATE ${GTK_INCLUDE_DIRS})
target_link_libraries(recipe-machine-2 ${GTK_LIBRARIES})

