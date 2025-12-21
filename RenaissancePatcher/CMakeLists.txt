cmake_minimum_required(VERSION 4.0)
project("RenaissancePatcher" CXX)

set (CMAKE_CXX_STANDARD 11)

add_executable(${PROJECT_NAME} WIN32 main.cpp peutils.cpp restools.cpp flashpatch.cpp)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "This should be build targetting a 32 bit system.")
endif ()

if (MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  target_sources(${PROJECT_NAME} PRIVATE res_cmake_msvc.rc)
  target_link_libraries(${PROJECT_NAME} dbghelp.lib gdi32.lib shell32.lib shlwapi.lib) 
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX /utf-8)
  target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)
  set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" /SUBSYSTEM:WINDOWS")
  
else ()
  target_sources(${PROJECT_NAME} PRIVATE res_mingw.rc)
  add_compile_definitions(_WIN32_WINNT_WIN2K)
  target_link_libraries(${PROJECT_NAME} dbghelp gdi32 shell32 shlwapi -static -static-libgcc -static-libstdc++) 
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror -O2 -municode)
endif ()