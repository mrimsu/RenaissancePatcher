cmake_minimum_required(VERSION 4.0)
project("RenaissancePatch" C)

set (CMAKE_C_STANDARD 99)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_library(${PROJECT_NAME} SHARED dllmain.c)


if (CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(STATUS "32-bit target detected, building")
else ()
    message(FATAL_ERROR "This patch is meant only for 32-bit targets as Agent itself is built for such")
endif ()


if (MSVC)
  message(STATUS "Building with MSVC")
  add_compile_definitions(_UNICODE=1 UNICODE=1)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  target_link_libraries(${PROJECT_NAME} wsock32.lib ws2_32.lib) 
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else ()
  message(STATUS "Building with MinGW")
  add_compile_definitions(_WIN32_WINNT_WIN2K)
  target_link_libraries(${PROJECT_NAME} wsock32 ws2_32) 
  target_compile_options(${PROJECT_NAME} PRIVATE -municode -Wall -Wextra -Werror -O2)
endif ()

set_target_properties(RenaissancePatch PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)