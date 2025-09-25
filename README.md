# README

打集创赛做的小东西，25年华北赛区二等奖，代码写的很垃圾，应该能跑通，跑不通我也懒得再改了，经供参考。

## 文件树结构

```
├── get_heat                                     # 热成像处理模块
│   ├── functions                                # 热成像功能实现源码
│   │   ├── MLX90640_API.cpp                     # MLX90640红外传感器API接口实现
│   │   ├── MLX90640_API.o                       # 编译生成的API目标文件
│   │   ├── MLX90640_I2C_Driver.cpp              # I2C通信驱动实现
│   │   ├── MLX90640_I2C_Driver.o                # 编译生成的I2C驱动目标文件
│   │   ├── pseudo_color.cpp                     # 温度数据伪彩色图像转换逻辑
│   │   └── pseudo_color.o                       # 伪彩色转换目标文件
│   ├── headers                                  # 头文件目录
│   │   ├── MLX90640_API.h                       # 红外传感器API声明
│   │   ├── MLX90640_I2C_Driver.h                # I2C驱动函数声明
│   │   └── pseudo_color.h                       # 伪彩色转换函数声明
│   ├── main.cpp                                 # 热成像模块主程序
│   ├── main.o                                   # 主程序编译目标文件
│   ├── makefile                                 # 编译规则文件
│   └── mlx_app                                  # 热成像应用程序可执行文件
│   
├── object_detection                             # 目标检测模块
│   ├── CMakeLists.txt                         
│   ├── build
│   │   ├── ...                               
│   │   ├── nanodet.bin
│   │   ├── nanodet.param						 # 权重与参数
│   │   └── nanodet		                         # 目标检测可执行文件
│   ├── main.cpp                                 # 目标检测主程序（视频捕获与推理入口）
│   ├── nanodet.cpp                              # Nanodet轻量级目标检测算法实现
│   └── nanodet.h                              
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

## 项目介绍

#### 超声避障（slaver_core_for_ultrasound）

利用OpenAMP开源项目及rpmsg通信协议，实现主核与从核之间的数据传输。在主核程序启动时，通过/dev/rpmsg0发送测距指令，从核通过UART2接口与MB1043交互，实现数据采集与解析，再返回距离数据到主核，为下一步无人机飞行提供决策依据，形成数据处理闭环。

#### 目标检测（object_detection）

主核配置并运行Phytium-Pi-OS系统，配置ncnn框架与opencv库，利用并改进轻量级目标检测算法nanodet，进行视频数据的捕获与处理，识别场景中的各种物体，为无人机飞行决策提供依据。

#### 热成像（get_heat）

系统利用MLX90640红外传感器实现热成像。利用迈来芯官方所给固件库进行在飞腾派上的移植，同时在通过红外传感器获取温度数据后使用插值算法与伪彩色转换，将其转化为温度矩阵和热像图并显示在屏幕上，为无人机预警提供依据。

## 使用方法

#### 超声避障（slaver_core_for_ultrasound）

在支持openamp`（linux+baremetal）`的系统镜像下测试：

将编译好的`openamp_core0.elf`置于`/lib/firmware`目录下，将测试程序`rpmsg.c`置于`~/`目录下，执行：

```bash
echo start > /sys/class/remoteproc/remoteproc0/state
echo rpmsg_chrdev > /sys/bus/rpmsg/devices/virtio0.rpmsg-openamp-demo-channel.-1.0/driver_override
modprobe rpmsg_char
```

然后编译运行`rpmsg.c`文件即可测试

#### 目标检测（object_detection）

本项目依赖opencv库和ncnn框架

进入build目录，运行`sudo ./nanodet 0 0`即可测试。

#### 热成像（get_heat）

直接`sudo ./mlx_app`
