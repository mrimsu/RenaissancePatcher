cmake_minimum_required(VERSION 4.0)
project("RenaissanceClientPatch" CXX)

add_subdirectory(RenaissancePatch RenaissancePatch\\Build\\bin)
add_subdirectory(RenaissancePatcher RenaissancePatcher\\Build\\bin)