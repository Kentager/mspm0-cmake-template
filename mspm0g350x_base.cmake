
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Maximum optimization for speed")
    add_compile_options(-Ofast)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    message(STATUS "Maximum optimization for speed, debug info included")
    add_compile_options(-Ofast -g)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "Maximum optimization for size")
    add_compile_options(-Os)
else ()
    message(STATUS "Minimal optimization, debug info included")
    add_compile_options(-Og -g)
endif ()


#######################
# 使用ARM GCC
#######################
# set(CMAKE_C_COMPILER_FORCED ON)
# set(CMAKE_CXX_COMPILER_FORCED ON)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(CMAKE_SIZE arm-none-eabi-size)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

#######################
# 设置SDK路径
#######################
set(SYSCONFIG_PATH  ${CMAKE_CURRENT_LIST_DIR}/SysConfig)

# 这一行要看你用SDK的安装路径去更改，改成你用的SDK的路径
set(MSPM0_SDK_PATH "E:/Ti/mspm0_sdk_2_10_00_04")

#######################
# 设置编译选项
#######################
add_compile_options(-mcpu=cortex-m0plus -march=armv6-m -mthumb -mfloat-abi=soft -Wall -gstrict-dwarf)   # 复制自m0 gcc例程的makefile
add_compile_options(-ffunction-sections -fdata-sections -fno-common -fmessage-length=0)
#######################
# 使能汇编文件处理
#######################
add_compile_options($<$<COMPILE_LANGUAGE:ASM>:-x$<SEMICOLON>assembler-with-cpp>)

#######################
# 添加include路径
#######################
include_directories(
    ${MSPM0_SDK_PATH}/source
    ${MSPM0_SDK_PATH}/source/third_party/CMSIS/Core/Include
    ${SYSCONFIG_PATH}
)

#######################
#添加设备定义
#######################
file(READ ${SYSCONFIG_PATH}/device.opt DEVICE_OPT)
add_definitions(${DEVICE_OPT})

#######################
# 设置链接选项
#######################
link_directories(${MSPM0_SDK_PATH}/source)

#######################
# 链接文件
#######################
add_link_options(-T${SYSCONFIG_PATH}/device_linker.lds)
add_link_options(-T${SYSCONFIG_PATH}/device.lds.genlibs)
add_link_options(-Wl,-gc-sections,--print-memory-usage,-Map=memory.map)  # 打印内存使用情况,生成map文件
add_link_options(-mcpu=cortex-m0plus -march=armv6-m -mthumb -static -lgcc -lc -lm -lnosys --specs=nano.specs -nostartfiles) # 复制自m0 gcc例程的makefile

file(GLOB_RECURSE MSPM0_SDK_SOURCES
    ${MSPM0_SDK_PATH}/source/ti/driverlib/*.c
)
file(GLOB_RECURSE MSPM0_STARTUP
    ${MSPM0_SDK_PATH}/source/ti/devices/msp/m0p/startup_system_files/gcc/startup_mspm0g350x_gcc.c
)

file(GLOB_RECURSE SYSCONFIG_SOURCES
    ${SYSCONFIG_PATH}/*.c
)

add_compile_options(-fno-exceptions)