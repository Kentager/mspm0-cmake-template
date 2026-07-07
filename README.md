# MSPM0G3507 FreeRTOS 小车工程
# 感谢 Kylin-6的开源工程项目 很酷
这是一个基于 **TI MSPM0G3507** 的嵌入式工程，使用 **CMake + Ninja + arm-none-eabi-gcc + clangd** 搭建开发环境，并在 **FreeRTOS** 上实现多任务调度。

当前工程已经不只是一个空模板，而是一个面向小车控制的完整示例：包含电机 PWM 驱动、编码器测速与里程计算、MPU9250 姿态角积分、OLED 菜单显示、按键交互、UART 调试输出，以及 OpenOCD 下载配置。

## 一、工程功能

- **FreeRTOS 多任务框架**
  - 主任务、LED 心跳任务、UART 输出任务、OLED 刷新任务、电机控制任务、MPU9250 采集任务并行运行。
- **双电机控制**
  - 支持左右电机 PWM 控制、正反转、刹车。
  - 电机应用层封装速度控制、角度控制、距离控制和急停接口。
- **编码器闭环基础**
  - 支持左右轮编码器中断计数。
  - 周期性计算速度和累计里程。
- **MPU9250 姿态采集**
  - 通过 I2C 读取加速度、陀螺仪、磁力计和温度数据。
  - 当前使用陀螺仪积分计算 pitch、roll、yaw 角度。
- **OLED 菜单系统**
  - 支持 OLED 显示姿态数据和菜单状态。
  - 菜单刷新逻辑已和按键处理逻辑分离。
- **按键输入**
  - 4 个按键通过中断触发，并带有基础消抖。
  - 按键事件通过 FreeRTOS Queue 发送给 OLED 应用层。
- **UART 调试输出**
  - 周期输出姿态角数据，方便串口观察调试。
- **OpenOCD 下载**
  - 提供 `openocd.cfg`，可配合 CMSIS-DAP 给 MSPM0G3507 下载程序。

## 二、项目结构

```text
MSPM0-CMAKE-GCC-TEMPLATE/
├── App/                     # 应用层代码
│   ├── Inc/
│   │   ├── motor_app.h      # 电机应用层接口
│   │   ├── oled_app.h       # OLED 应用层接口
│   │   └── oled_menu.h      # OLED 菜单管理接口
│   └── Src/
│       ├── motor_app.c      # 电机速度/角度/距离控制逻辑
│       ├── oled_app.c       # OLED 页面与业务显示
│       └── oled_menu.c      # 菜单渲染与按键导航
├── HardWare/                # 硬件驱动层代码
│   ├── Inc/
│   │   ├── encoder.h        # 编码器接口
│   │   ├── key.h            # 按键接口
│   │   ├── madgwick.h       # 姿态解算相关接口
│   │   ├── motor.h          # 电机底层驱动接口
│   │   ├── mpu9250.h        # MPU9250 驱动接口
│   │   ├── oled.h           # OLED 底层驱动接口
│   │   ├── oled_font.h      # OLED 字库
│   │   ├── speed_pid.h      # 速度 PID 接口
│   │   └── uart.h           # UART 接口
│   └── Src/
│       ├── encoder.c
│       ├── key.c
│       ├── madgwick.c
│       ├── motor.c
│       ├── mpu9250.c
│       ├── oled.c
│       ├── speed_pid.c
│       └── uart.c
├── Includes/
│   └── FreeRTOSConfig.h     # FreeRTOS 配置文件
├── Sources/
│   ├── main.c               # 主函数、任务创建、中断处理
│   └── sysmem.c             # 内存相关支持文件
├── SysConfig/               # TI SysConfig 生成文件和链接脚本
├── .vscode/                 # VS Code 配置
├── CMakeLists.txt           # 顶层 CMake 构建脚本
├── mspm0g350x_base.cmake    # MSPM0G350x 工具链、SDK、链接配置
├── openocd.cfg              # OpenOCD 下载配置
└── README.md                # 项目说明
```

## 三、开发环境

建议准备以下工具：

- CMake
- Ninja
- clangd
- arm-none-eabi-gcc
- OpenOCD（建议 0.12 及以上，低版本可能不支持 TI MSPM0）
- TI MSPM0 SDK
- TI SysConfig
- VS Code
- Git

### 1. 安装 CMake

下载地址：<https://github.com/Kitware/CMake/releases/>

下载后将 `bin` 目录添加到系统环境变量，例如：

```text
D:\DevEnv\cmake-4.3.0-rc3-windows-x86_64\bin
```

可以通过下面命令验证：

```bash
cmake --version
```

### 2. 安装 Ninja

下载地址：<https://github.com/ninja-build/ninja/releases/>

将 `ninja.exe` 所在目录添加到系统环境变量，然后验证：

```bash
ninja --version
```

### 3. 安装 clangd

下载地址：<https://github.com/llvm/llvm-project/releases/>

安装完成后一般会自动添加环境变量，如果没有生效，可以手动将 LLVM 的 `bin` 目录加入环境变量。

验证命令：

```bash
clangd --version
```

### 4. 安装 arm-none-eabi-gcc

如果之前开发过 STM32，电脑里大概率已经有这个工具链，可以先检查：

```bash
arm-none-eabi-gcc --version
```

下载地址：<https://developer.arm.com/downloads/-/gnu-rm>

安装后同样需要将工具链的 `bin` 目录添加到环境变量。

### 5. 安装 OpenOCD

下载地址：<https://github.com/xpack-dev-tools/openocd-xpack/releases>

验证命令：

```bash
openocd --version
```

> 注意：MSPM0 建议使用 OpenOCD 0.12 及以上版本。部分较老教程里的 OpenOCD 版本可能不支持 TI 芯片。

### 6. 安装 TI 环境

MSPM0 SDK 下载地址：<https://www.ti.com/tool/MSPM0-SDK#downloads>

SysConfig 下载地址：<https://www.ti.com.cn/tool/cn/SYSCONFIG>

建议统一安装到 TI 目录下，例如：

```text
D:\TI\mspm0_sdk_2_10_00_04
D:\TI\sysconfig_1.27.0
```

## 四、VS Code 配置

建议安装以下插件：

- clangd
- Chinese (Simplified) 简体中文
- Clang-Format（可选，用于格式化）
- CMake Tools
- openocd-tools

### clangd 配置

在 clangd 插件参数中添加：

```text
--compile-commands-dir=${workspaceFolder}/build
```

如果同时安装了微软官方 C/C++ 插件，右下角可能会提示 IntelliSense 冲突，建议直接禁用微软 C/C++ 插件的 IntelliSense，使用 clangd 作为主要代码补全和跳转工具。

## 五、工程配置

### 1. 修改 MSPM0 SDK 路径

打开 `mspm0g350x_base.cmake`，根据自己电脑上的 SDK 安装路径修改：

```cmake
set(MSPM0_SDK_PATH "E:/Ti/mspm0_sdk_2_10_00_04")
```

### 2. 修改 FreeRTOS 路径

打开 `CMakeLists.txt`，根据自己电脑上的 MSPM0 SDK 安装路径修改 FreeRTOS 源码路径：

```cmake
set(FREERTOS_PATH "E:/Ti/mspm0_sdk_2_10_00_04/kernel/freertos/Source")
```

> 本工程直接使用 MSPM0 SDK 中自带的 FreeRTOS V11.2.0 源码。

## 六、编译工程

用 VS Code 打开本项目文件夹。

如果 CMake Tools 弹出工具包选择，选择 ARM GCC 对应工具链，等待 CMake 自动配置完成。

也可以在终端手动执行：

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

编译成功后会在 `build` 目录下生成：

```text
MSPM0_FreeRTOS_Template.elf
```

如果终端输出退出代码为 0，则说明编译正常。

## 七、下载程序

工程已经提供 `openocd.cfg`：

```cfg
source [find interface/cmsis-dap.cfg]
source [find target/ti/mspm0.cfg]

adapter speed 4000

init
targets
halt

program build/MSPM0_FreeRTOS_Template.elf verify reset exit
```

使用步骤：

1. 连接 MSPM0G3507 开发板和 CMSIS-DAP / DAPLink。
2. 确保工程已经成功编译并生成 ELF 文件。
3. 使用 VS Code 的 openocd-tools，或者在终端运行 OpenOCD。
4. 选择当前工程下的 `openocd.cfg`。
5. 下载成功后芯片会复位并运行程序。

## 八、SysConfig 使用说明

本工程的 TI SysConfig 相关文件放在 `SysConfig/` 目录下。

如果需要修改引脚、外设、时钟等配置：

1. 打开 TI SysConfig。
2. 选择当前使用的 MSPM0 SDK。
3. 打开 `SysConfig/` 目录下的 `.syscfg` 配置文件。
4. 修改配置后重新生成代码。
5. 回到 VS Code 重新编译工程。

## 九、当前任务说明

`Sources/main.c` 中创建了多个 FreeRTOS 任务：

| 任务 | 作用 |
| --- | --- |
| `vMainTask` | 自动运行逻辑，周期设置小车目标速度和目标角度 |
| `vBlinkTask` | LED 心跳闪烁 |
| `vUARTTask` | 串口周期输出 pitch、roll、yaw |
| `vOLEDTask` | OLED 初始化、刷新显示、处理按键队列 |
| `vMotorTask` | 电机应用层初始化和 10ms 周期控制 |
| `vMPU9250Task` | MPU9250 初始化、陀螺仪校准和姿态角积分 |

外部中断 `GROUP1_IRQHandler` 负责处理：

- 左右编码器脉冲
- 4 个按键输入
- 按键消抖
- 按键事件发送到 FreeRTOS Queue

## 十、可能遇到的问题

### 1. 代码爆红

如果 clangd 配置完成后代码仍然爆红，可以按下 `Ctrl + Shift + P`，输入：

```text
clangd: Restart Language Server
```

执行后 clangd 会重新读取 `build/compile_commands.json`，代码补全和跳转一般就能恢复正常。

### 2. 找不到 SDK 或 FreeRTOS

检查以下两个路径是否和本机一致：

- `mspm0g350x_base.cmake` 中的 `MSPM0_SDK_PATH`
- `CMakeLists.txt` 中的 `FREERTOS_PATH`

### 3. OpenOCD 无法识别芯片

优先确认：

- OpenOCD 版本是否为 0.12 及以上。
- `openocd.cfg` 是否使用了 `target/ti/mspm0.cfg`。
- DAPLink / CMSIS-DAP 是否连接正常。
- 芯片供电和 SWD 接线是否正确。

## 十一、后续可扩展方向

- 将姿态解算从简单陀螺仪积分升级为 Madgwick / 互补滤波。
- 完善速度 PID 参数整定。
- 增加距离闭环、转向闭环和路线规划。
- 增加 OLED 菜单参数在线调节功能。
- 将 UART 数据格式改为更方便上位机解析的协议。
