#ifndef DETECTION_MODULE_H
#define DETECTION_MODULE_H

#include <opencv2/opencv.hpp>
#include "nanodet.h"

struct object_rect {
    int x;
    int y;
    int width;
    int height;
};

class DetectionModule {
private:
    NanoDet* detector;
    cv::VideoCapture cap;
    bool initialized;
    bool camera_opened;
    
    // 辅助函数
    int resize_uniform(cv::Mat& src, cv::Mat& dst, cv::Size dst_size, object_rect& effect_area);
    void draw_bboxes(const cv::Mat& bgr, const std::vector<BoxInfo>& bboxes, object_rect effect_roi);
    
public:
    DetectionModule();
    ~DetectionModule();
    
    // 初始化目标检测模块
    bool initialize(const char* param_path = "./nanodet.param", 
                   const char* bin_path = "./nanodet.bin", 
                   bool useGPU = false);
    
    // 打开摄像头
    bool openCamera(int cam_id = 0);
    
    // 处理一帧图像并显示检测结果
    bool processFrame();
    
    // 清理资源
    void cleanup();
    
    // 检查是否已初始化
    bool isInitialized() const { return initialized; }
    
    // 检查摄像头是否已打开
    bool isCameraOpened() const { return camera_opened; }
};

#endif // DETECTION_MODULE_H
