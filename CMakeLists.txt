cmake_minimum_required(VERSION 3.7)

project(6502Emulator)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add subdirectories
add_subdirectory(6502/6502lib)
add_subdirectory(6502/6502Test)
