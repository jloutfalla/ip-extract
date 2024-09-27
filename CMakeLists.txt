cmake_minimum_required(VERSION 3.12)

project("IP Extractor" LANGUAGES C)

set(BIN_NAME "ip-extract")


add_executable(${BIN_NAME})
target_compile_options
set_target_properties(${BIN_NAME} PROPERTIES C_EXTENSIONS OFF)

add_subdirectory(src)
