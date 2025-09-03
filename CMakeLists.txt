cmake_minimum_required(VERSION 3.16)
project(pio_emu)

set(CMAKE_CXX_STANDARD 20)

# Common source files
set(COMMON_SOURCES
        src/PioStateMachine.cpp
        src/PioVariableRegistry.cpp
        src/PioStateMachine.h
        src/TimingDiagram.cpp
        src/TimingDiagram.h
        src/logger/Logger.cpp
        src/logger/Logger.h
)

# Main executable src/main.cpp
add_executable(pio_emu
        src/gui/imgui_test.cpp
        ${COMMON_SOURCES}
)

# Find required packages
# fmt
find_package(fmt CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)

# doctest
find_package(doctest CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE doctest::doctest)

# imgui
find_package(imgui CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
    imgui::imgui 
    d3d11
)

#if (WIN32)
#    set_target_properties(${PROJECT_NAME} PROPERTIES
#        WIN32_EXECUTABLE TRUE
#    )
#endif()


# Enable testing
enable_testing()

# List of all test targets
set(PIO_TESTS
        set
        irq
        out_tick
        in
        gpioset
        doSideSet
        set_tick
        tick
        jmp
        irq_tick
        mov_tick
        pull_tick
        push_tick
        in_tick
        wait_tick
        ws2812
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