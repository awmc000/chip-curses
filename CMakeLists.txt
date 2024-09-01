cmake_minimum_required(VERSION 3.29)

project(curse-chip VERSION 0.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)


include_directories(include)

add_executable(cursechip src/main.cpp src/chip8.cpp)
target_link_libraries(cursechip -lncurses)

# find_package(Catch2 3 REQUIRED)
# add_executable(chiptest src/chip8.cpp test/test.cpp)
# target_link_libraries(chiptest PRIVATE Catch2::Catch2WithMain)
