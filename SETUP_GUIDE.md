# MSPM0G3507 CMake + FreeRTOS 从零搭建指南

本文档介绍如何从零搭建一个基于 MSPM0G3507 的 CMake + GCC + FreeRTOS 工程，不依赖 TI CCS 官方 IDE。

## 为什么不用 CCS？

TI 官方推荐使用 **CCS (Code Composer Studio)** 开发 MSPM0 系列，但 CCS 存在以下不足：

- 基于 Eclipse，界面老旧、启动慢
- 不支持 clangd，代码补全和跳转体验差
- 工程配置与 IDE 深度绑定，不便于团队协作和 CI/CD

本工程采用 **CMake + Ninja + arm-none-eabi-gcc + clangd + VS Code** 的现代工具链：

- 纯命令行编译，不依赖任何 IDE
- clangd 提供优秀的代码补全、跳转和诊断
- CMake 跨平台，Windows / Linux / macOS 均可编译
- 版本控制友好，所有配置都是文本文件

---

## 项目介绍

### 项目概述

本项目是一个面向 **智能小车** 的完整嵌入式工程，以 **TI MSPM0G3507**（ARM Cortex-M0+, 64MHz, 128KB Flash, 32KB RAM）为主控芯片，运行 **FreeRTOS** 实时操作系统，实现双电机闭环控制、姿态解算、循迹传感器采集、OLED 菜单交互等功能。

项目最初为 2025 年全国大学生电子设计竞赛（电赛）控制题设计，实现了小车自主行驶、精确转向、距离控制等比赛需求。

### 硬件架构

```
                    MSPM0G3507 (Cortex-M0+, 64MHz)
                    ┌─────────────────────────────┐
                    │                             │
  [左右编码器] ─────►│ GPIO (中断)   PWM ──────────┼──► [TB6612 电机驱动] ──► [左右电机]
                    │                             │
  [MPU9250]  ◄─────►│ I2C_1                       │
                    │                             │
  [OLED 0.96"]◄─────│ I2C_0                       │
                    │                             │
  [8路循迹传感器] ──►│ GPIO (输入)                 │
                    │                             │
  [4个按键]  ───────►│ GPIO (中断)                 │
                    │                             │
  [UART 调试] ◄────►│ UART                        │
                    │                             │
                    └─────────────────────────────┘
```

| 硬件 | 型号 / 规格 | 接口 | 说明 |
|------|-------------|------|------|
| 主控 | MSPM0G3507 | — | TI MSPM0 系列，Cortex-M0+ |
| 电机驱动 | TB6612FNG | PWM + GPIO | 双路 H 桥，20kHz PWM |
| 编码器 | 500 线增量式 | GPIO 中断 | 减速比 56:1，轮径 65mm |
| IMU | MPU9250 | I2C (0x68) | 9 轴：加速度计 + 陀螺仪 + 磁力计 |
| OLED | 0.96" 128×64 | I2C (0x3C) | SSD1306 驱动，显示姿态和菜单 |
| 循迹传感器 | 8 路红外 | GPIO | 从左到右排列，检测黑线 |
| 按键 | 4 个轻触按键 | GPIO 中断 | 用于菜单导航和任务触发 |

### 软件架构

工程采用 **分层架构**，自底向上分为三层：

```
┌──────────────────────────────────────────────────┐
│                   App 应用层                      │
│   motor_app  │  oled_app  │  oled_menu            │
├──────────────────────────────────────────────────┤
│               HardWare 硬件驱动层                  │
│  motor │ encoder │ speed_pid │ mpu9250 │ oled     │
│  key   │ uart    │ super_sensor │ madgwick        │
├──────────────────────────────────────────────────┤
│          SysConfig + MSPM0 SDK DriverLib           │
│       GPIO │ I2C │ UART │ Timer (PWM)             │
└──────────────────────────────────────────────────┘
```

### 模块详解

#### 1. 电机驱动 — `HardWare/motor`

TB6612 双路电机驱动，通过 PWM 控制转速，GPIO 控制方向。

- PWM 频率 20kHz（`64MHz / 3200 = 20kHz`），超出人耳范围
- 支持前进、后退、刹车三种模式
- 左右电机共用一个 PWM 定时器的两个通道

```c
Motor_Init();                         // 初始化
Motor_SetSpeed(&motor_left, 1600);    // 设置 50% 占空比
Motor_Brake(&motor_right);            // 刹车
```

#### 2. 编码器 — `HardWare/encoder`

500 线增量式编码器，通过 GPIO 中断实现 4 倍频计数。

- 每转脉冲数：`500 × 56 × 4 = 112000`（考虑减速比和 4 倍频）
- 周期性计算线速度（m/s）和累计里程（m）
- 轮子周长 `π × 65mm ≈ 204.2mm`

```c
Encoder_UpdateMetrics(&encoder_left, 10);  // 每 10ms 调用
float speed = Encoder_GetVelocity(&encoder_left);  // m/s
float dist = Encoder_GetDistance(&encoder_left);   // 米
```

#### 3. 速度 PID — `HardWare/speed_pid`

增量式 PID 控制器，用于电机速度闭环。

- 支持输出限幅和积分限幅（防积分饱和）
- 左右轮各一个独立 PID 实例
- 输出值直接映射为 PWM 占空比

```c
Speed_PID_Init();
float output = Speed_PID_Compute(&pid_left, target_speed, actual_speed);
```

#### 4. MPU9250 姿态采集 — `HardWare/mpu9250`

9 轴 IMU 驱动，通过 I2C 读取加速度计、陀螺仪和 AK8963 磁力计数据。

- 陀螺仪量程可配置（250/500/1000/2000 dps）
- 加速度计量程可配置（2/4/8/16g）
- 支持数字低通滤波（DLPF）
- 内置陀螺仪零偏校准
- 当前使用简单陀螺仪积分计算 pitch/roll/yaw（可升级为 Madgwick 滤波）

```c
MPU9250_Init();              // 初始化 + WHO_AM_I 校验
MPU9250_CalibrateGyro();     // 静止状态零偏校准
MPU9250_ReadAll(&data);      // 读取全部数据
```

#### 5. 循迹传感器 — `HardWare/super_sensor`

8 路红外循迹传感器阵列，从左到右排列，用于检测地面黑线。

- 8 通道：`LEFT_OUTER → LEFT_3 → LEFT_2 → LEFT_INNER → RIGHT_INNER → RIGHT_2 → RIGHT_3 → RIGHT_OUTER`
- 计算左右差速用于转向控制
- 最大差速限制 `0.9 × 1.025 m/s`

```c
Super_Sensor_Init(&superSensor);
Super_Sensor_Update(&superSensor);
float diff = Super_Sensor_GetDiffSpeed(&superSensor);  // 差速
```

#### 6. 电机应用层 — `App/motor_app`

在硬件驱动之上封装的高层控制接口，提供三种控制模式：

| 模式 | 说明 | API |
|------|------|-----|
| `SPEED_MODE` | 设定目标速度持续行驶 | `Motor_App_SetSpeed(left, right)` |
| `ANGLE_MODE` | 设定目标角度转向 | `Motor_App_SetTargetYaw(yaw)` |
| `SENSOR_MODE` | 循迹传感器引导行驶 | 自动根据传感器差速转向 |

同时支持距离控制：

```c
Motor_App_Drive(1.3, 1.3, 0.4);   // 以 0.4m/s 前进 1.3 米
while(!Motor_App_IsReached());      // 等待到达
Motor_App_Brake();                  // 急停
```

#### 7. OLED 显示 — `App/oled_app` + `App/oled_menu`

OLED 菜单系统，支持多页面显示和按键导航。

- 显示姿态角（pitch / roll / yaw）
- 显示 8 路传感器状态
- 显示倒计时计时器（比赛计时）
- 4 个按键控制菜单切换和任务触发

菜单按键功能：

| 按键 | 功能 |
|------|------|
| KEY_0 | 执行任务 0 |
| KEY_1 | 执行任务 1 |
| KEY_2 | 执行任务 2 |
| KEY_3 | 执行任务 3 / 停止 |

#### 8. 按键 — `HardWare/key`

4 个轻触按键，通过 GPIO 中断触发，带 500ms 消抖。

- 按键事件通过 FreeRTOS Queue 传递给 OLED 任务
- 支持 KEY_0 ~ KEY_3 四种事件

#### 9. UART 调试 — `HardWare/uart`

UART 调试输出，周期发送 YAW 角度数据。

- 格式：`YAW:xxx.xx\r\n`（20ms 周期）
- 方便上位机实时监控姿态

### FreeRTOS 任务架构

系统共运行 7 个 FreeRTOS 任务：

```
┌─────────────────┐     ┌──────────────────┐
│  vMPU9250Task   │     │   vSensorTask    │
│   优先级: 4     │     │   优先级: 3      │
│   10ms 周期     │     │   10ms 周期      │
│  读取 IMU 数据  │     │  采集循迹传感器  │
│  陀螺仪积分     │     │  计算差速        │
└────────┬────────┘     └────────┬─────────┘
         │ pitch/roll/yaw        │ sensor_values
         ▼                       ▼
┌─────────────────┐     ┌──────────────────┐
│   vMotorTask    │◄────│   vMainTask      │
│   优先级: 3     │     │   优先级: 1      │
│   10ms 周期     │     │   接收任务指令   │
│  PID 计算       │     │  控制行驶逻辑    │
│  驱动电机       │     │                  │
└─────────────────┘     └──────────────────┘
         ▲                        │ Job_e (Queue)
         │                        ▼
┌────────┴────────┐     ┌──────────────────┐
│   vOLEDTask     │     │   vBlinkTask     │
│   优先级: 2     │     │   优先级: 1      │
│  刷新 OLED      │     │   1s 周期        │
│  处理按键队列   │     │  LED 心跳        │
│  显示倒计时     │     │                  │
└─────────────────┘     └──────────────────┘

         ┌──────────────────┐
         │   vUARTTask      │
         │   优先级: 1      │
         │   20ms 周期      │
         │  串口输出 YAW    │
         └──────────────────┘

         ┌──────────────────┐
 中断 →  │ GROUP1_IRQHandler│
         │  编码器脉冲计数  │
         │  按键消抖        │
         │  发送 Queue 消息 │
         └──────────────────┘
```

任务间通信使用 FreeRTOS Queue：

| Queue | 生产者 | 消费者 | 数据 |
|-------|--------|--------|------|
| `xKeyQueue` | 中断 | vOLEDTask | `key_e` 按键事件 |
| `xJobQueue` | vOLEDTask | vMainTask | `Job_e` 任务指令 |

### 数据流

```
编码器中断 ──► 脉冲计数 ──► Encoder_UpdateMetrics ──► 速度/里程
                                                        │
MPU9250 ──► ReadAll ──► 陀螺仪积分 ──► pitch/roll/yaw ─┤
                                                        ▼
传感器 GPIO ──► Super_Sensor_Update ──► 差速 ──────► Motor_App_Update
                                                        │
                                                        ▼
                                                   PID 输出
                                                        │
                                                        ▼
                                                   PWM 占空比 ──► 电机
```

---

## 一、安装开发工具

### 1.1 安装 ARM GCC 工具链

下载地址：<https://developer.arm.com/downloads/-/gnu-rm>

安装后将 `bin` 目录添加到系统 PATH。

验证：

```bash
arm-none-eabi-gcc --version
```

### 1.2 安装 CMake

下载地址：<https://github.com/Kitware/CMake/releases/>

安装时勾选 "Add to PATH"。

验证：

```bash
cmake --version
```

### 1.3 安装 Ninja

下载地址：<https://github.com/ninja-build/ninja/releases/>

将 `ninja.exe` 所在目录添加到 PATH。

验证：

```bash
ninja --version
```

### 1.4 安装 OpenOCD

下载地址：<https://github.com/xpack-dev-tools/openocd-xpack/releases>

> MSPM0 需要 OpenOCD ≥ 0.12，低版本不支持 TI 芯片。

验证：

```bash
openocd --version
```

### 1.5 安装 clangd（可选，强烈推荐）

下载地址：<https://github.com/llvm/llvm-project/releases/>

安装后自动添加环境变量，用于 VS Code 代码补全。

### 1.6 安装 TI MSPM0 SDK

下载地址：<https://www.ti.com/tool/MSPM0-SDK#downloads>

建议安装到固定目录，例如：

```
E:\Ti\mspm0_sdk_2_10_00_04
```

> 记住安装路径，后续 CMake 配置需要用到。

### 1.7 安装 TI SysConfig

下载地址：<https://www.ti.com.cn/tool/cn/SYSCONFIG>

用于图形化配置芯片引脚、外设、时钟，生成初始化代码。

---

## 二、创建工程目录结构

新建一个空文件夹，按以下结构创建目录和文件：

```
my-mspm0-project/
├── App/                     # 你的应用层代码
│   ├── Inc/
│   └── Src/
├── HardWare/                # 硬件驱动层
│   ├── Inc/
│   └── Src/
├── Includes/                # FreeRTOS 配置
│   └── FreeRTOSConfig.h
├── Sources/                 # 主程序
│   ├── main.c
│   └── sysmem.c
├── SysConfig/               # SysConfig 生成文件（下一步创建）
├── CMakeLists.txt           # 顶层 CMake（下一步创建）
├── mspm0g350x_base.cmake    # 工具链配置（下一步创建）
└── openocd.cfg              # 烧录配置（下一步创建）
```

---

## 三、使用 SysConfig 生成外设配置

SysConfig 负责生成芯片的外设初始化代码，包括 GPIO、UART、I2C、Timer、时钟树等。

### 3.1 创建 SysConfig 项目

1. 打开 TI SysConfig
2. 选择 **MSPM0 SDK** 作为产品
3. 选择 **MSPM0G3507** 作为器件
4. 选择 **GCC** 作为工具链

### 3.2 配置外设

在 SysConfig 图形界面中添加你需要的外设：

- **GPIO** — LED、按键引脚
- **Timer** — PWM 输出（电机控制）
- **UART** — 调试串口
- **I2C** — MPU9250、OLED 通信

### 3.3 导出到工程

配置完成后，将生成的文件导出到 `SysConfig/` 目录下，应包含：

```
SysConfig/
├── MSPM0_FreeRTOS_Template.syscfg   # SysConfig 配置文件
├── ti_msp_dl_config.c               # 外设初始化代码
├── ti_msp_dl_config.h               # 外设配置头文件
├── device.opt                        # 编译宏定义
├── device_linker.lds                 # 链接脚本
├── device.lds.genlibs                # 链接库引用
└── device.cmd.genlibs                # 命令文件
```

> 这些文件已包含在本项目中，可直接使用。如需修改引脚或外设，再用 SysConfig 重新生成。

---

## 四、编写 CMake 构建脚本

### 4.1 工具链配置 — `mspm0g350x_base.cmake`

这个文件定义了交叉编译器、SDK 路径、编译选项和链接配置：

```cmake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)

# 编译优化选项
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    add_compile_options(-Ofast)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    add_compile_options(-Ofast -g)
else ()
    add_compile_options(-Og -g)
endif ()

# ARM GCC 工具链
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(CMAKE_SIZE arm-none-eabi-size)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# ★ 修改为你的 SDK 路径
set(MSPM0_SDK_PATH "E:/Ti/mspm0_sdk_2_10_00_04")
set(SYSCONFIG_PATH ${CMAKE_CURRENT_LIST_DIR}/SysConfig)

# CPU 编译选项（Cortex-M0+）
add_compile_options(-mcpu=cortex-m0plus -march=armv6-m -mthumb -mfloat-abi=soft -Wall)
add_compile_options(-ffunction-sections -fdata-sections -fno-common -fmessage-length=0)

# SDK 头文件
include_directories(
    ${MSPM0_SDK_PATH}/source
    ${MSPM0_SDK_PATH}/source/third_party/CMSIS/Core/Include
    ${SYSCONFIG_PATH}
)

# 设备宏定义
file(READ ${SYSCONFIG_PATH}/device.opt DEVICE_OPT)
add_definitions(${DEVICE_OPT})

# 链接配置
link_directories(${MSPM0_SDK_PATH}/source)
add_link_options(-T${SYSCONFIG_PATH}/device_linker.lds)
add_link_options(-T${SYSCONFIG_PATH}/device.lds.genlibs)
add_link_options(-Wl,-gc-sections,--print-memory-usage,-Map=memory.map)
add_link_options(-mcpu=cortex-m0plus -march=armv6-m -mthumb -static -lgcc -lc -lm -lnosys --specs=nano.specs -nostartfiles)

# SDK 驱动库源文件
file(GLOB_RECURSE MSPM0_SDK_SOURCES ${MSPM0_SDK_PATH}/source/ti/driverlib/*.c)

# 启动文件
file(GLOB_RECURSE MSPM0_STARTUP
    ${MSPM0_SDK_PATH}/source/ti/devices/msp/m0p/startup_system_files/gcc/startup_mspm0g350x_gcc.c)

# SysConfig 生成源文件
file(GLOB_RECURSE SYSCONFIG_SOURCES ${SYSCONFIG_PATH}/*.c)
```

> 关键点：`-mcpu=cortex-m0plus` 指定目标 CPU 为 Cortex-M0+，`--specs=nano.specs` 使用 newlib-nano 减小固件体积。

### 4.2 顶层 CMake — `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.30)
include(mspm0g350x_base.cmake)

project(MSPM0_FreeRTOS_Template C CXX ASM)

# ★ 修改为你的 SDK 路径下的 FreeRTOS
set(FREERTOS_PATH "E:/Ti/mspm0_sdk_2_10_00_04/kernel/freertos/Source")

include_directories(
    ./Includes
    ${FREERTOS_PATH}/include
    ${FREERTOS_PATH}/portable/GCC/ARM_CM0/
    ./HardWare/Inc
    ./App/Inc
)

# 收集源文件
file(GLOB_RECURSE C_SOURCES "./Sources/*.c")
file(GLOB FREERTOS_C_SOURCES "${FREERTOS_PATH}/*.c")
file(GLOB FREERTOS_MEMMANG "${FREERTOS_PATH}/portable/MemMang/heap_4.c")
file(GLOB_RECURSE FREERTOS_PORT "${FREERTOS_PATH}/portable/GCC/ARM_CM0/*.c")
file(GLOB_RECURSE HARDWARE_SRC "./HardWare/Src/*.c")
file(GLOB_RECURSE APP_SRC "./App/Src/*.c")

add_executable(${CMAKE_PROJECT_NAME}.elf
    ${SYSCONFIG_SOURCES}
    ${MSPM0_SDK_SOURCES}
    ${MSPM0_STARTUP}
    ${C_SOURCES}
    ${FREERTOS_C_SOURCES}
    ${FREERTOS_MEMMANG}
    ${FREERTOS_PORT}
    ${HARDWARE_SRC}
    ${APP_SRC}
)

target_link_libraries(${CMAKE_PROJECT_NAME}.elf m)
target_link_options(${CMAKE_PROJECT_NAME}.elf PRIVATE
    -Wl,--gc-sections -Wl,--print-memory-usage -Wl,-Map=memory.map -lm)

# 生成 compile_commands.json（供 clangd 使用）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
```

> 关键点：FreeRTOS 源码直接使用 MSPM0 SDK 自带的 V11.2.0，无需单独下载。`CMAKE_EXPORT_COMPILE_COMMANDS ON` 生成编译数据库供 clangd 读取。

---

## 五、添加 FreeRTOS 配置

在 `Includes/` 目录下创建 `FreeRTOSConfig.h`，这是 FreeRTOS 的核心配置文件，需要根据 MSPM0G3507 的硬件特性设置：

- `configCPU_CLOCK_HZ` — CPU 主频
- `configTICK_RATE_HZ` — Tick 频率（通常 1000）
- `configTOTAL_HEAP_SIZE` — 堆内存大小
- `configMINIMAL_STACK_SIZE` — 最小栈大小
- 中断优先级相关宏

> 本项目的 `Includes/FreeRTOSConfig.h` 已配置好，可直接参考或复用。

---

## 六、创建烧录配置

在项目根目录创建 `openocd.cfg`：

```cfg
source [find interface/cmsis-dap.cfg]
source [find target/ti/mspm0.cfg]

adapter speed 4000

init
targets
halt

program build/MSPM0_FreeRTOS_Template.elf verify reset exit
```

---

## 七、编写 main.c

`Sources/main.c` 是程序入口，负责初始化系统和创建 FreeRTOS 任务：

```c
#include "FreeRTOS.h"
#include "task.h"
#include "ti_msp_dl_config.h"

// 你的任务函数
static void vBlinkTask(void *pvParameters) {
    for (;;) {
        DL_GPIO_togglePins(GPIOA, DL_GPIO_PIN_0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int main(void) {
    SYSCFG_DL_init();    // SysConfig 生成的初始化函数

    xTaskCreate(vBlinkTask, "Blink", 128, NULL, 1, NULL);
    vTaskStartScheduler();

    for (;;) {}
}
```

> `SYSCFG_DL_init()` 是 SysConfig 生成的外设初始化函数，必须在启动调度器前调用。

---

## 八、编译与烧录

### 8.1 修改路径

克隆或复制本工程后，修改两处路径：

- `mspm0g350x_base.cmake` 中的 `MSPM0_SDK_PATH`
- `CMakeLists.txt` 中的 `FREERTOS_PATH`

### 8.2 编译

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

编译成功输出 `build/MSPM0_FreeRTOS_Template.elf`。

### 8.3 烧录

连接 CMSIS-DAP 调试器到开发板 SWD 接口，执行：

```bash
openocd -f openocd.cfg
```

---

## 九、VS Code 配置

### 推荐插件

| 插件 | 用途 |
|------|------|
| clangd | 代码补全、跳转、诊断 |
| CMake Tools | CMake 构建管理 |
| openocd-tools | 烧录固件 |

### clangd 配置

在 clangd 插件参数中添加：

```
--compile-commands-dir=${workspaceFolder}/build
```

> 如同时安装了微软 C/C++ 插件，建议禁用其 IntelliSense，使用 clangd 替代。

---

## 十、常见问题

### 代码爆红

`Ctrl+Shift+P` → `clangd: Restart Language Server`

### 找不到 SDK 或 FreeRTOS

检查 `mspm0g350x_base.cmake` 中 `MSPM0_SDK_PATH` 和 `CMakeLists.txt` 中 `FREERTOS_PATH` 是否正确。

### OpenOCD 无法识别芯片

- 确认 OpenOCD ≥ 0.12
- 检查 SWD 接线和供电
- 确认 `openocd.cfg` 使用 `target/ti/mspm0.cfg`

### 编译报错 "No such file or directory"

路径中不要包含中文或空格，SDK 建议安装到纯英文路径。

### 链接报错 "undefined reference"

检查 CMake 是否遗漏了某些源文件。可用 `file(GLOB_RECURSE ...)` 确认所有 `.c` 文件都被收集。

---

## 十一、下一步

- 参考本项目 `HardWare/` 目录的驱动代码，编写你自己的外设驱动
- 参考 `App/` 目录的应用层代码，构建业务逻辑
- 使用 SysConfig 修改引脚和外设配置，重新生成代码
- 阅读 [README.md](README.md) 了解完整功能特性
