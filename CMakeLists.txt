cmake_minimum_required(VERSION 3.16)
project(pio_emu)

set(CMAKE_CXX_STANDARD 20)

# Common source files
set(COMMON_SOURCES
        src/PioStateMachine.cpp
        src/PioStateMachine.h
        src/logger/Logger.cpp
        src/logger/Logger.h
)

# Main executable
add_executable(pio_emu
        src/main.cpp
        ${COMMON_SOURCES}
)

# Find required packages
find_package(fmt CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)

find_package(doctest CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE doctest::doctest)

# Enable testing
enable_testing()

# Create tests directory structure if it doesn't exist
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/core)

# List of all test targets
set(PIO_TESTS
        set
        irq
        out
        in
        gpioset
        doSideSet
        set_tick
        tick
        jmp
)

# Create test executables from the list
foreach (TEST_NAME ${PIO_TESTS})
    add_executable(test_pio_emu_${TEST_NAME}
            tests/core/test_pio_emu_${TEST_NAME}.cpp
            ${COMMON_SOURCES}
    )
    target_link_libraries(test_pio_emu_${TEST_NAME} PRIVATE fmt::fmt doctest::doctest)
    add_test(NAME test_pio_emu_${TEST_NAME} COMMAND test_pio_emu_${TEST_NAME})
endforeach ()

# Logger test executable
add_executable(test_logger
        tests/logger_test.cpp
        src/logger/Logger.cpp
        src/logger/Logger.h
)
target_link_libraries(test_logger PRIVATE fmt::fmt)