cmake_minimum_required(VERSION 4.0)
project("RenaissancePatcher" CXX)

set (CMAKE_CXX_STANDARD 11)

add_executable(${PROJECT_NAME} WIN32 main.cpp peutils.cpp restools.cpp bytepatch.cpp)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "This should be build targetting a 32 bit system.")
endif ()

target_sources(${PROJECT_NAME} PRIVATE res_mingw.rc)
add_compile_definitions(_WIN32_WINNT_WIN2K)
target_link_libraries(${PROJECT_NAME} dbghelp gdi32 shell32 shlwapi winmm version -static -static-libgcc -static-libstdc++) 
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -fpermissive -municode)