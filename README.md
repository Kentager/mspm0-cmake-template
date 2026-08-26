# MSPM0G3507 FreeRTOS Car Project

基于 **TI MSPM0G3507** 的嵌入式智能小车工程，使用 **CMake + Ninja + arm-none-eabi-gcc + clangd** 搭建现代开发环境，运行 **FreeRTOS** 实现多任务调度。

> 感谢 [Kylin-6](https://github.com/Kylin-6) 的开源工程，为本项目提供了宝贵的参考。

## 一、功能特性

- **FreeRTOS 多任务框架** — 主控制、LED 心跳、UART 调试、OLED 显示、电机控制、IMU 采集、传感器采集等多任务并行运行
- **双电机闭环控制** — PWM 驱动、正反转、刹车，支持速度 / 角度 / 距离多种控制模式
- **编码器测速与里程计** — 左右轮编码器中断计数，周期性计算速度和累计里程
- **MPU9250 姿态采集** — I2C 读取加速度计、陀螺仪、磁力计，陀螺仪积分计算 pitch / roll / yaw
- **OLED 菜单系统** — 多页面显示姿态数据与传感器状态，支持按键导航
- **按键交互** — 4 个按键中断触发 + 消抖，通过 FreeRTOS Queue 传递事件
- **UART 调试输出** — 周期输出 YAW 数据，方便串口监控
- **OpenOCD 烧录** — 提供配置文件，支持 CMSIS-DAP 下载

## 二、环境依赖

| 工具 | 说明 |
|------|------|
| CMake | ≥ 3.30 |
| Ninja | 构建系统 |
| arm-none-eabi-gcc | ARM 交叉编译工具链 |
| clangd | 代码补全与跳转（可选） |
| OpenOCD | ≥ 0.12，用于烧录 |
| TI MSPM0 SDK | 设备驱动库 |
| TI SysConfig | 引脚与外设配置 |
| VS Code | 推荐 IDE |

### 安装步骤

1. **CMake** — 下载：<https://github.com/Kitware/CMake/releases/>，将 `bin` 目录加入 PATH

2. **Ninja** — 下载：<https://github.com/ninja-build/ninja/releases/>，将 `ninja.exe` 所在目录加入 PATH

3. **arm-none-eabi-gcc** — 下载：<https://developer.arm.com/downloads/-/gnu-rm>，将工具链 `bin` 目录加入 PATH

4. **clangd**（可选）— 下载：<https://github.com/llvm/llvm-project/releases/>，安装后自动添加环境变量

5. **OpenOCD** — 下载：<https://github.com/xpack-dev-tools/openocd-xpack/releases>，≥ 0.12

6. **TI MSPM0 SDK** — 下载：<https://www.ti.com/tool/MSPM0-SDK#downloads>

7. **TI SysConfig** — 下载：<https://www.ti.com.cn/tool/cn/SYSCONFIG>

验证安装：

```bash
cmake --version
ninja --version
arm-none-eabi-gcc --version
openocd --version
```

## 三、VS Code 配置

推荐插件：
- **clangd** — 代码补全（需在插件参数中添加 `--compile-commands-dir=${workspaceFolder}/build`）
- **CMake Tools** — CMake 构建管理
- **openocd-tools** — 烧录工具

> 如同时安装了微软 C/C++ 插件，建议禁用其 IntelliSense，使用 clangd 替代。

## 四、项目结构

```
MSPM0-CMAKE-GCC-TEMPLATE/
├── App/                     # 应用层
│   ├── Inc/
│   │   ├── motor_app.h      # 电机应用层接口
│   │   ├── oled_app.h       # OLED 应用层接口
│   │   └── oled_menu.h      # OLED 菜单管理
│   └── Src/
│       ├── motor_app.c      # 电机控制逻辑
│       ├── oled_app.c       # OLED 页面与业务显示
│       └── oled_menu.c      # 菜单渲染与导航
├── HardWare/                # 硬件驱动层
│   ├── Inc/
│   │   ├── encoder.h        # 编码器
│   │   ├── key.h            # 按键
│   │   ├── madgwick.h       # 姿态解算
│   │   ├── motor.h          # 电机驱动
│   │   ├── mpu9250.h        # MPU9250 驱动
│   │   ├── oled.h           # OLED 驱动
│   │   ├── oled_font.h      # 字库
│   │   ├── speed_pid.h      # 速度 PID
│   │   ├── super_sensor.h   # 传感器阵列
│   │   └── uart.h           # UART
│   └── Src/
│       ├── encoder.c
│       ├── key.c
│       ├── madgwick.c
│       ├── motor.c
│       ├── mpu9250.c
│       ├── oled.c
│       ├── speed_pid.c
│       ├── super_sensor.c
│       └── uart.c
├── Includes/
│   └── FreeRTOSConfig.h     # FreeRTOS 配置
├── Sources/
│   ├── main.c               # 主函数与任务创建
│   └── sysmem.c             # 内存支持
├── SysConfig/               # TI SysConfig 生成文件 & 链接脚本
├── .vscode/                 # VS Code 配置
├── CMakeLists.txt           # 顶层 CMake
├── mspm0g350x_base.cmake    # 工具链 & SDK 配置
└── openocd.cfg              # OpenOCD 烧录配置
```

## 五、工程配置

克隆项目后需要修改两处路径：

### 1. SDK 路径

编辑 `mspm0g350x_base.cmake`：

```cmake
set(MSPM0_SDK_PATH "你的SDK路径/mspm0_sdk_2_10_00_04")
```

### 2. FreeRTOS 路径

编辑 `CMakeLists.txt`：

```cmake
set(FREERTOS_PATH "你的SDK路径/mspm0_sdk_2_10_00_04/kernel/freertos/Source")
```

> FreeRTOS V11.2.0 源码来自 MSPM0 SDK，无需单独下载。

## 六、编译

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

编译成功后生成 `build/MSPM0_FreeRTOS_Template.elf`。

## 七、烧录

1. 连接 MSPM0G3507 开发板与 CMSIS-DAP / DAPLink
2. 确保已编译生成 ELF 文件
3. 在终端执行 OpenOCD：

```bash
openocd -f openocd.cfg
```

或使用 VS Code 的 openocd-tools 插件选择 `openocd.cfg` 执行。

## 八、FreeRTOS 任务

| 任务 | 优先级 | 说明 |
|------|--------|------|
| `vMainTask` | 1 | 主控制逻辑，接收任务指令驱动小车 |
| `vBlinkTask` | 1 | LED 心跳指示 |
| `vUARTTask` | 1 | 串口周期输出 YAW 数据 |
| `vOLEDTask` | 2 | OLED 显示刷新与按键处理 |
| `vMotorTask` | 3 | 电机 10ms 周期控制 |
| `vSensorTask` | 3 | 传感器阵列采集 |
| `vMPU9250Task` | 4 | IMU 数据采集与姿态解算 |

中断处理（`GROUP1_IRQHandler`）：
- 左右编码器脉冲计数
- 4 个按键输入（带消抖）
- 按键事件通过 FreeRTOS Queue 传递

## 九、常见问题

**代码爆红？**
按 `Ctrl+Shift+P` → `clangd: Restart Language Server`

**找不到 SDK 或 FreeRTOS？**
检查 `mspm0g350x_base.cmake` 中的 `MSPM0_SDK_PATH` 和 `CMakeLists.txt` 中的 `FREERTOS_PATH` 是否正确。

**OpenOCD 无法识别芯片？**
- 确认 OpenOCD ≥ 0.12
- 检查 SWD 接线和芯片供电
- 确认使用 `target/ti/mspm0.cfg`

## 十、SysConfig 使用说明

本工程的 TI SysConfig 相关文件放在 `SysConfig/` 目录下。

如需修改引脚、外设、时钟等配置：

1. 打开 TI SysConfig
2. 选择当前使用的 MSPM0 SDK
3. 打开 `SysConfig/` 目录下的 `.syscfg` 配置文件
4. 修改配置后重新生成代码
5. 回到 VS Code 重新编译工程

## 十一、后续可扩展方向

- 将姿态解算从简单陀螺仪积分升级为 Madgwick / 互补滤波
- 完善速度 PID 参数整定
- 增加距离闭环、转向闭环和路线规划
- 增加 OLED 菜单参数在线调节功能
- 将 UART 数据格式改为更方便上位机解析的协议

## License

MIT License
