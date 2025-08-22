#include "thermal_module.h"
#include <cstdio>
#include <unistd.h>

ThermalModule::ThermalModule() 
    : emissivity(0.95), delay_us(1000000 / 4), initialized(false) {
    // 构造函数
}

ThermalModule::~ThermalModule() {
    cleanup();
}

bool ThermalModule::initialize() {
    if (initialized) {
        return true;
    }
    
    // 1) 初始化 I2C 驱动
    if (MLX90640_I2CInit() != 0) {
        printf("I2C Init failed\n");
        return false;
    }
    
    // 2) 设置帧率与模式
    int status = MLX90640_SetRefreshRate(MLX90640_ADDR, RefreshRate);
    if (status != 0) {
        printf("SetRefreshRate error %d\n", status);
        return false;
    }
    
    status = MLX90640_SetChessMode(MLX90640_ADDR);
    if (status != 0) {
        printf("SetChessMode error %d\n", status);
        return false;
    }

    // 3) 读取并解析校准参数
    status = MLX90640_DumpEE(MLX90640_ADDR, eeMLX90640);
    if (status != 0) {
        printf("DumpEE error %d\n", status);
        return false;
    }
    
    status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
    if (status != 0) {
        printf("ExtractParameters error %d\n", status);
        return false;
    }

    // 4) 设置显示窗口
    cv::namedWindow("MLX90640 Thermal", cv::WINDOW_NORMAL);
    cv::resizeWindow("MLX90640 Thermal", 800, 600);
    
    initialized = true;
    printf("Thermal module initialized successfully\n");
    return true;
}

bool ThermalModule::processFrame() {
    if (!initialized) {
        printf("Thermal module not initialized\n");
        return false;
    }
    
    // 读取一帧原始数据
    int status = MLX90640_GetFrameData(MLX90640_ADDR, frame);
    if (status < 0) {
        printf("GetFrame Error: %d\r\n", status);
        return false;
    }
    
    float vdd = MLX90640_GetVdd(frame, &mlx90640);
    float Ta = MLX90640_GetTa(frame, &mlx90640); 
    float tr = Ta - TA_SHIFT;

    // 计算像素点温度
    MLX90640_CalculateTo(frame, &mlx90640, emissivity, tr, mlx90640To);
    
    // 找到最小和最大温度值
    float minT = mlx90640To[0], maxT = mlx90640To[0];
    for (int i = 1; i < 768; ++i) {
        if (mlx90640To[i] < minT) minT = mlx90640To[i];
        if (mlx90640To[i] > maxT) maxT = mlx90640To[i];
    }
    float range = maxT - minT;
    if (range < 1e-3) range = 1.0f;

    // 生成彩色图像
    cv::Mat img(24, 32, CV_8UC3);
    for (int i = 0; i < 24; ++i) {
        for (int j = 0; j < 32; ++j) {
            int idx = i * 32 + j;
            float t = mlx90640To[idx];
            uint8_t gray = static_cast<uint8_t>(255.0f * (t - minT) / range);
            uint8_t r, g, b;
            GrayToPseColor(0, gray, r, g, b);
            img.at<cv::Vec3b>(i, j) = cv::Vec3b(b, g, r); // OpenCV为BGR
        }
    }
    
    // 插值放大（使用 Lanczos4 算法提升边缘锐利度）
    cv::Mat img_big;
    cv::resize(img, img_big, cv::Size(320, 240), 0, 0, cv::INTER_LANCZOS4);
    cv::imshow("MLX90640 Thermal", img_big);
    
    // 检查是否有按键按下
    int key = cv::waitKey(1);
    if (key == 27) { // ESC键
        return false;
    }
    
    usleep(delay_us);
    return true;
}

void ThermalModule::cleanup() {
    if (initialized) {
        cv::destroyWindow("MLX90640 Thermal");
        initialized = false;
        printf("Thermal module cleaned up\n");
    }
}
