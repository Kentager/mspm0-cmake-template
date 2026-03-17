cmake_minimum_required(VERSION 3.30)
include(mspm0g350x_base.cmake)

project(MSPM0_FreeRTOS_Template C CXX ASM)

include_directories(
    ./Includes
    ./FreeRTOS/include
    ./FreeRTOS/portable/GCC/ARM_CM0/
)

file(GLOB_RECURSE C_SOURCES "./Sources/*.c")
file(GLOB FREERTOS_C_SOURCES "./FreeRTOS/*.c")
file(GLOB FREERTOS_MEMMANG_C_SOURCES "./FreeRTOS/portable/MemMang/heap_4.c")
file(GLOB_RECURSE FREERTOS_PORTABLE_C_SOURCES "./FreeRTOS/portable/GCC/ARM_CM0/*.c")

add_executable(${CMAKE_PROJECT_NAME}.elf
    ${SYSCONFIG_SOURCES}
    ${MSPM0_SDK_SOURCES}
    ${MSPM0_STARTUP}
    ${C_SOURCES}
)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)