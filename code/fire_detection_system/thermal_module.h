#ifndef THERMAL_MODULE_H
#define THERMAL_MODULE_H

#include <opencv2/opencv.hpp>
#include "MLX90640_I2C_Driver.h"
#include "MLX90640_API.h"
#include "pseudo_color.h"

class ThermalModule {
private:
    static const uint16_t MLX90640_ADDR = 0x33;
    static const uint16_t RefreshRate = 0x03; // FPS4HZ
    static const uint16_t TA_SHIFT = 8;
    
    uint16_t eeMLX90640[832];
    float mlx90640To[768];
    uint16_t frame[834];
    float emissivity;
    paramsMLX90640 mlx90640;
    
    useconds_t delay_us;
    bool initialized;
    
public:
    ThermalModule();
    ~ThermalModule();
    
    // 初始化热像图模块
    bool initialize();
    
    // 获取一帧热像图数据并显示
    bool processFrame();
    
    // 清理资源
    void cleanup();
    
    // 检查是否已初始化
    bool isInitialized() const { return initialized; }
};

#endif // THERMAL_MODULE_H
