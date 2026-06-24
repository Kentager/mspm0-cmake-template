cmake_minimum_required(VERSION 3.30)
include(mspm0g350x_base.cmake)

project(MSPM0_FreeRTOS_Template C CXX ASM)

# Use SDK's FreeRTOS V11.2.0 source (tested with MSPM0)
set(FREERTOS_PATH "E:/Ti/mspm0_sdk_2_10_00_04/kernel/freertos/Source")


include_directories(
    ./Includes
    ${FREERTOS_PATH}/include
    ${FREERTOS_PATH}/portable/GCC/ARM_CM0/
    ./HardWare/Inc
    ./App/Inc
)

file(GLOB_RECURSE C_SOURCES "./Sources/*.c")
file(GLOB FREERTOS_C_SOURCES "${FREERTOS_PATH}/*.c")
file(GLOB FREERTOS_MEMMANG_C_SOURCES "${FREERTOS_PATH}/portable/MemMang/heap_4.c")
file(GLOB_RECURSE FREERTOS_PORTABLE_C_SOURCES "${FREERTOS_PATH}/portable/GCC/ARM_CM0/*.c")
file(GLOB_RECURSE HARDWARE_SOURCES "./HardWare/Src/*.c")
file(GLOB_RECURSE App_SOURCES "./App/Src/*.c")

add_executable(${CMAKE_PROJECT_NAME}.elf
    ${SYSCONFIG_SOURCES}
    ${MSPM0_SDK_SOURCES}
    ${MSPM0_STARTUP}
    ${C_SOURCES}
    ${FREERTOS_C_SOURCES}
    ${FREERTOS_MEMMANG_C_SOURCES}
    ${FREERTOS_PORTABLE_C_SOURCES}
    ${HARDWARE_SOURCES}
    ${App_SOURCES}
)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
