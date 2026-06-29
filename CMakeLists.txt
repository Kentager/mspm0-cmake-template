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


# 链接数学库
target_link_libraries(${CMAKE_PROJECT_NAME}.elf
    m                          # ← 链接数学库 libm.a
)

# 确保链接器选项包含数学库（双重保险）
target_link_options(${CMAKE_PROJECT_NAME}.elf PRIVATE
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -Wl,-Map=memory.map
    -lm                        # ← 显式再写一次
)

# 如果使用 nano.specs，确保数学库正确链接
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lm")


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
