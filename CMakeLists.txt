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

# Add test executable for SET functionality
add_executable(test_pio_emu_set
        tests/test_pio_emu_set.cpp
        src/PioStateMachine.cpp
        src/PioStateMachine.h
)
target_link_libraries(test_pio_emu_set PRIVATE fmt::fmt doctest::doctest)
add_test(NAME test_pio_emu_set COMMAND test_pio_emu_set)

# Add test executable for IRQ functionality
add_executable(test_pio_emu_irq
        tests/test_pio_emu_irq.cpp
        src/PioStateMachine.cpp
        src/PioStateMachine.h
)
target_link_libraries(test_pio_emu_irq PRIVATE fmt::fmt doctest::doctest)
add_test(NAME test_pio_emu_irq COMMAND test_pio_emu_irq)

# Add test executable for OUT functionality
add_executable(test_pio_emu_out
        tests/test_pio_emu_out.cpp
        src/PioStateMachine.cpp
        src/PioStateMachine.h
)
target_link_libraries(test_pio_emu_out PRIVATE fmt::fmt doctest::doctest)
add_test(NAME test_pio_emu_out COMMAND test_pio_emu_out)