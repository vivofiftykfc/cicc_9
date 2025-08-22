# 火灾检测系统构建指南

本项目整合了热成像检测和目标检测两个模块，提供了多种构建方式。

## 系统要求

### 操作系统
- Linux (Ubuntu 18.04+, CentOS 7+, 或其他主流发行版)

### 依赖库
- OpenCV 4.x
- NCNN (深度学习推理框架)
- CMake 3.10+ (如果使用CMake构建)
- pkg-config
- GCC 7+ 或 Clang 6+
- i2c-tools (用于热成像传感器)

## 快速开始

### 方法1: 使用自动化脚本 (推荐)

```bash
# 给脚本执行权限
chmod +x build.sh

# 安装系统依赖
./build.sh -i

# 编译项目
./build.sh

# 编译并运行
./build.sh --run
```

### 方法2: 使用Makefile

```bash
# 检查依赖
make check-deps

# 编译
make

# 运行
make run
```

### 方法3: 使用CMake

```bash
# 给脚本执行权限
chmod +x cmake_build.sh

# 编译
./cmake_build.sh

# 或者手动使用CMake
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## 详细使用说明

### 自动化构建脚本 (build.sh)

这是最推荐的构建方式，提供了完整的自动化流程。

#### 安装依赖
```bash
./build.sh -i
```

#### 编译选项
```bash
# 标准编译
./build.sh

# 清理后编译
./build.sh -c

# 编译调试版本
./build.sh -d

# 编译发布版本
./build.sh -r

# 只编译热成像模块
./build.sh -t

# 只编译目标检测模块
./build.sh -o

# 准备检测模型
./build.sh -b

# 编译并运行
./build.sh --run
```

### Makefile选项

```bash
# 显示所有可用目标
make help

# 检查依赖
make check-deps

# 编译各模块
make thermal          # 只编译热成像模块
make detection        # 只编译目标检测模块
make system           # 只编译系统主模块

# 不同构建类型
make debug            # 调试版本
make release          # 发布版本

# 清理
make clean            # 清理编译文件
make distclean        # 深度清理

# 运行
make run              # 编译并运行
```

### CMake选项

```bash
# 使用构建脚本
./cmake_build.sh -d   # 调试构建
./cmake_build.sh -r   # 发布构建
./cmake_build.sh -c   # 清理后构建
./cmake_build.sh --run # 构建并运行

# 手动CMake
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## 模型文件准备

目标检测模块需要NanoDet模型文件：

1. 下载模型文件：
   - `nanodet.param`
   - `nanodet.bin`

2. 将文件放置在：
   ```
   object_detection/build/nanodet.param
   object_detection/build/nanodet.bin
   ```

3. 或者运行：
   ```bash
   ./build.sh -b  # 准备模型目录
   ```

## 项目结构

```
fire_detection_system/
├── build.sh                    # 主构建脚本
├── cmake_build.sh             # CMake构建脚本
├── Makefile                   # 主Makefile
├── CMakeLists.txt             # CMake配置
├── fire_detection_system.cpp  # 主程序
├── thermal_module.h/.cpp      # 热成像模块
├── detection_module.h/.cpp    # 目标检测模块
├── get_heat/                  # 热成像相关文件
│   ├── headers/               # 头文件
│   └── functions/             # 实现文件
└── object_detection/          # 目标检测相关文件
    ├── nanodet.h/.cpp         # NanoDet实现
    └── build/                 # 模型文件目录
```

## 常见问题

### 1. OpenCV未找到
```bash
# Ubuntu/Debian
sudo apt-get install libopencv-dev libopencv-contrib-dev

# CentOS/RHEL
sudo yum install opencv-devel
```

### 2. NCNN未找到
```bash
# 手动编译安装NCNN
git clone https://github.com/Tencent/ncnn.git
cd ncnn
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

### 3. I2C权限问题
```bash
# 添加用户到i2c组
sudo usermod -a -G i2c $USER
# 重新登录生效
```

### 4. 模型文件缺失
确保在 `object_detection/build/` 目录下有：
- `nanodet.param`
- `nanodet.bin`

### 5. 编译错误
```bash
# 清理后重新编译
./build.sh -c
./build.sh -r

# 或者
make distclean
make
```

## 运行程序

编译成功后，运行：
```bash
./fire_detection_system
```

程序提供交互式菜单：
- `t` - 切换到热成像模式
- `d` - 切换到目标检测模式  
- `q` - 退出系统
- `h` - 显示帮助

在运行模式中按 `ESC` 键返回主菜单。

## 开发说明

### 添加新功能
1. 在相应的模块文件中添加代码
2. 更新 Makefile 和 CMakeLists.txt 的依赖关系
3. 重新编译测试

### 调试
```bash
# 编译调试版本
./build.sh -d
# 或
make debug

# 使用GDB调试
gdb ./fire_detection_system
```

### 性能优化
```bash
# 编译优化版本
./build.sh -r
# 或  
make release
```

## 许可证

请参考各个模块的原始许可证：
- MLX90640 API: Apache License 2.0
- NanoDet: Apache License 2.0
- OpenCV: Apache License 2.0
