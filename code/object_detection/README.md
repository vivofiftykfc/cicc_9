# README

## 文件树结构

```
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
```

## 项目介绍

#### 目标检测（object_detection）

主核配置并运行Phytium-Pi-OS系统，配置ncnn框架与opencv库，利用并改进轻量级目标检测算法nanodet，进行视频数据的捕获与处理，识别场景中的各种物体，为无人机飞行决策提供依据。

## 使用方法

#### 目标检测（object_detection）

##### 接入摄像头

找一个USB摄像头插上就好

##### 配置环境

本项目依赖opencv库和ncnn框架

在飞腾派等linux开发板下具体配置方法见下述链接：

https://github.com/opencv/opencv

https://github.com/Tencent/ncnn/wiki/how-to-build#build-for-linux

具体操作为：

opencv：

```bash
git clone https://github.com/opencv/opencv.git
git clone https://github.com/opencv/opencv_contrib.git  # 可选，包含额外的模块
cd opencv
mkdir build
cd build
cmake -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=/usr/local ..
make -j$(nproc)
sudo make install
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

ncnn：

以debian系linux系统为例：

```bash
sudo apt install build-essential git cmake libprotobuf-dev protobuf-compiler libomp-dev libopencv-dev
git clone --recursive https://github.com/Tencent/ncnn.git
cd ncnn
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DNCNN_VULKAN=OFF -DNCNN_BUILD_EXAMPLES=ON ..
#测试知飞腾派好像没有vulkan，如果开启相关选项，会用软件模拟vulkan，反而会拖慢推理速度。本例程默认关闭vulkan，仅用CPU进行推理
make -j$(nproc)
export ncnn_DIR=YOUR_NCNN_PATH/build/install/lib/cmake/ncnn
```

##### 加载运行

进入build目录，运行`./nanodet 0 0`即可测试。

可能问题：

- MobaXterm在超级用户下无法转发识别图像，但是普通用户下`xclock`测试UI转发正常：`sudo -E ./nanodet 0 0`
- 权限不够无法访问摄像头：`sudo ./nanodet 0 0`

可选从源码编译：

```bash
mkdir build
cd build
cmake ..
make
```