cmake_minimum_required(VERSION 3.16)
project(pio_emu)

set(CMAKE_CXX_STANDARD 20)

# Common source files
set(COMMON_SOURCES
        src/PioStateMachine.cpp
        src/PioVariableRegistry.cpp
        src/PioStateMachine.h
        src/logger/Logger.cpp
        src/logger/Logger.h
        src/iniparse.h
)

# Main executable src/main.cpp
add_executable(pio_emu_gui
        src/gui/GuiMain.cpp
        src/gui/PioStateMachineApp.cpp
        src/gui/PioStateMachineApp.h
        src/gui/ImGuiStyle.h
        ${COMMON_SOURCES}
)

add_executable(pio_emu_cli
        src/main.cpp
        ${COMMON_SOURCES}
)

# Find required packages
find_package(fmt CONFIG REQUIRED)
find_package(doctest CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# pio_emu_cli
target_link_libraries(${PROJECT_NAME}_cli PRIVATE 
    fmt::fmt 
    doctest::doctest
)

# pio_emu_gui
target_link_libraries(${PROJECT_NAME}_gui PRIVATE 
    imgui::imgui 
    d3d11
    fmt::fmt
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