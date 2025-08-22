# README

## 文件树结构

```
├── fire_detection_system
│   ├── CMakeLists.txt           # CMake 编译配置
│   ├── fire_detection_system.cpp# 主控制程序
│   ├── thermal_module.h         # 热像图模块头文件
│   ├── thermal_module.cpp       # 热像图模块实现
│   ├── detection_module.h       # 目标检测模块头文件
│   └── detection_module.cpp     # 目标检测模块实现
│   ├── get_heat                                     # 热成像处理模块
│   │   ├── functions                                # 热成像功能实现源码
│   │   │   ├── MLX90640_API.cpp                     # MLX90640红外传感器API接口实现
│   │   │   ├── MLX90640_I2C_Driver.cpp              # I2C通信驱动实现
│   │   │   └── pseudo_color.cpp                     # 温度数据伪彩色图像转换逻辑
│   │   ├── headers                                  # 头文件目录
│   │   │   ├── MLX90640_API.h                       # 红外传感器API声明
│   │   │   ├── MLX90640_I2C_Driver.h                # I2C驱动函数声明
│   │   │   └── pseudo_color.h                       # 伪彩色转换函数声明
│   │   ├── main.cpp                                 # 热成像模块主程序
│   │   ├── makefile                                 # 测试编译规则文件
│   │   └── mlx_app                                  # 热成像应用程序可执行文件│   
├── object_detection                             # 目标检测模块                 
│   │   ├── build
│   │   │   ├── ...                               
│   │   │   ├── nanodet.bin
│   │   │   ├── nanodet.param						 # 权重与参数
│   │   │   └── nanodet		                         # 目标检测可执行文件
│   │   ├── main.cpp                                 # 测试目标检测主程序（视频捕获与推理入口）
│   │   ├── nanodet.cpp                              # Nanodet轻量级目标检测算法实现
│   │   └── nanodet.h                              
│   
├── slaver_core_for_ultrasound                   # 超声避障模块（飞腾派双核协同）
│   ├── openamp_core0.elf                        # 从核可执行文件（主核通过OpenAMP加载）
│   ├── rpmsg.c                                  # 主核控制端代码（rpmsg协议实现核间通信）
│   ├── slaver_core_for_ultrasound               # 从核固件工程目录
│   │   ├── Kconfig                              # 内核配置选项（编译参数定义）
│   │   ├── phytiumpi_aarch64_firefly_openamp_core0.elf  # 最终从核固件（用于裸机程序）
│   │   ├── phytiumpi_aarch64_firefly_openamp_core0.map  # 内存映射文件（地址分配）
│   │   ├── build                               
│   │   │   └── drivers                          # 驱动编译结果
│   │   │       └── fpl011                       # ARM PL011 UART驱动
│   │   │           ├── fpl011.o                 # UART设备驱动目标文件
│   │   │           ├── fpl011_hw.o              # 硬件寄存器操作目标文件
│   │   │           └── fpl011_intr.o            # 中断处理逻辑目标文件
│   │   ├── common                               # 公共配置文件
│   │   │   ├── memory_layout.h                  # 内存分区定义（共享内存地址配置）
│   │   │   └── openamp_configs.h                # OpenAMP框架参数配置
│   │   ├── configs                              # 平台配置文件
│   │   │   └── phytiumpi_aarch64_firefly_openamp_core0.config  
│   │   ├── ft_openamp.ld                        # 链接脚本（从核代码/数据段内存布局）
│   │   ├── inc                                 
│   │   │   └── slaver_00_example.h              # 从核超声数据采集函数声明
│   │   ├── main.c                               # 从核程序入口
│   │   ├── makefile                            
│   │   └── src                                  
│   │       └── slaver_00_example.c              # 从核主逻辑（UART数据采集与解析实现）
└── 工程文件结构.txt 
```

## 超声避障（slaver_core_for_ultrasound）

### 模块介绍

利用OpenAMP开源项目及rpmsg通信协议，实现主核与从核之间的数据传输。在主核程序启动时，通过/dev/rpmsg0发送测距指令，从核通过UART2接口与MB1043交互，实现数据采集与解析，再返回距离数据到主核，为下一步无人机飞行提供决策依据，形成数据处理闭环。

### 使用方法

在支持openamp`（linux+baremetal）`的系统镜像下测试：

将编译好的`openamp_core0.elf`置于`/lib/firmware`目录下，将测试程序`rpmsg.c`置于`~/`目录下，执行：

```bash
echo start > /sys/class/remoteproc/remoteproc0/state
echo rpmsg_chrdev > /sys/bus/rpmsg/devices/virtio0.rpmsg-openamp-demo-channel.-1.0/driver_override
modprobe rpmsg_char
```

然后编译运行`rpmsg.c`文件即可测试

## 火灾识别（fire_detection_system）

### 模块简介

本模块集成了热像图检测和目标检测功能，主要特性：

- 热像图和目标检测分别封装为 `ThermalModule` 与 `DetectionModule` 类  
- 支持运行时通过交互式菜单动态切换检测模式  
- 自动完成模块初始化、运行循环及资源清理  

### 依赖

- CMake ≥ 3.4  
- C++11 编译器 (g++, clang++ 或 MSVC)  
- OpenCV  
- ncnn  
- OpenMP  

### 编译

```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

编译完成后会生成可执行文件 `FireDetectionSystem`。

### 运行

```bash
# Linux 下可能需要 sudo 权限以访问摄像头
sudo ./FireDetectionSystem
```

启动后，控制台将显示菜单：

- 输入 `t` 切换到热像图模式  
- 输入 `d` 切换到目标检测模式  
- 输入 `h` 显示帮助菜单  
- 输入 `q` 退出系统  
- 在检测模式中按 `ESC` 返回主菜单  
