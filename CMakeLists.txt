cmake_minimum_required(VERSION 3.16)
project(pio_emu)

set(CMAKE_CXX_STANDARD 20)

add_executable(pio_emu
        src/main.cpp
        src/PioStateMachine.cpp
        src/PioStateMachine.h
)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)
