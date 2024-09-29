cmake_minimum_required(VERSION 3.12)

project("IP Extractor" LANGUAGES C)

set(BIN_NAME "ip-extract")

add_executable(${BIN_NAME})
set_target_properties(${BIN_NAME} PROPERTIES C_EXTENSIONS OFF)

include(TestBigEndian)
set(IS_BIG_ENDIAN 0)
test_big_endian(IS_BIG_ENDIAN)

# message(WARNING $<IF:${IS_BIG_ENDIAN},BIG_ENDIAN=1,BIG_ENDIAN=0>)
if(IS_BIG_ENDIAN)
  target_compile_definitions(${BIN_NAME} PRIVATE "-DIS_BIG_ENDIAN=1")
else()
  target_compile_definitions(${BIN_NAME} PRIVATE "-DIS_BIG_ENDIAN=0")
endif()

# target_compile_definitions(${BIN_NAME} PRIVATE "-DBIG_ENDIAN=${IS_BIG_ENDIAN}")
# target_compile_definitions(${BIN_NAME} PRIVATE $<IF:${IS_BIG_ENDIAN},BIG_ENDIAN=1,BIG_ENDIAN=0>)


add_subdirectory(src)
