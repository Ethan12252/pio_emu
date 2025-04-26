cmake_minimum_required(VERSION 3.16)
project(pio_emu)

set(CMAKE_CXX_STANDARD 20)

# Main executable
add_executable(pio_emu
        src/main.cpp
        src/PioStateMachine.cpp
        src/PioStateMachine.h
)

# Find required packages
find_package(fmt CONFIG REQUIRED)
find_package(doctest CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)

# Enable testing
enable_testing()

# Add test executable
add_executable(pio_emu_tests
        tests/test_main.cpp
        src/PioStateMachine.cpp
        src/PioStateMachine.h
)

target_link_libraries(pio_emu_tests PRIVATE fmt::fmt doctest::doctest)
add_test(NAME pio_emu_tests COMMAND pio_emu_tests)