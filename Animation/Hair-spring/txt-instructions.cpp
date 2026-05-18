cmake_minimum_required(VERSION 3.18)
project(HairPhysics CXX)

set(CMAKE_CXX_STANDARD 17)

# Header-only library target
add_library(hair_physics INTERFACE)
target_include_directories(hair_physics INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# --- Optional test/demo executable ---
# add_executable(hair_demo main_demo.cpp)
# target_link_libraries(hair_demo PRIVATE hair_physics)
