#include <iostream>
#include <opencv2/opencv.hpp>
#include "thermal_module.h"
#include "detection_module.h"

enum class Mode {
    THERMAL,
    DETECTION,
    EXIT
};

class FireDetectionSystem {
private:
    ThermalModule thermal_module;
    DetectionModule detection_module;
    Mode current_mode;
    bool system_running;
    
public:
    FireDetectionSystem() : current_mode(Mode::THERMAL), system_running(true) {}
    
    bool initialize() {
        std::cout << "=== 火灾检测系统初始化 ===" << std::endl;
        
        // 初始化热像图模块
        std::cout << "正在初始化热像图模块..." << std::endl;
        if (!thermal_module.initialize()) {
            std::cout << "热像图模块初始化失败!" << std::endl;
            return false;
        }
        
        // 初始化目标检测模块
        std::cout << "正在初始化目标检测模块..." << std::endl;
        if (!detection_module.initialize("../object_detection/build/nanodet.param", 
                                        "../object_detection/build/nanodet.bin", false)) {
            std::cout << "目标检测模块初始化失败!" << std::endl;
            return false;
        }
        
        // 打开摄像头
        std::cout << "正在打开摄像头..." << std::endl;
        if (!detection_module.openCamera(0)) {
            std::cout << "摄像头打开失败!" << std::endl;
            return false;
        }
        
        std::cout << "系统初始化完成!" << std::endl;
        return true;
    }
    
    void printMenu() {
        std::cout << "\\n=== 火灾检测系统控制菜单 ===" << std::endl;
        std::cout << "当前模式: " << (current_mode == Mode::THERMAL ? "热像图检测" : 
                     current_mode == Mode::DETECTION ? "目标检测" : "未知") << std::endl;
        std::cout << "控制指令:" << std::endl;
        std::cout << "  't' - 切换到热像图模式" << std::endl;
        std::cout << "  'd' - 切换到目标检测模式" << std::endl;
        std::cout << "  'q' - 退出系统" << std::endl;
        std::cout << "  'h' - 显示帮助菜单" << std::endl;
        std::cout << "  ESC - 在运行模式中按ESC键也可以返回菜单" << std::endl;
        std::cout << "请输入指令: ";
    }
    
    void runThermalMode() {
        std::cout << "\\n=== 开始热像图检测模式 ===" << std::endl;
        std::cout << "按ESC键退出当前模式..." << std::endl;
        
        while (system_running && current_mode == Mode::THERMAL) {
            if (!thermal_module.processFrame()) {
                std::cout << "热像图模式已停止" << std::endl;
                break;
            }
        }
    }
    
    void runDetectionMode() {
        std::cout << "\\n=== 开始目标检测模式 ===" << std::endl;
        std::cout << "按ESC键退出当前模式..." << std::endl;
        
        while (system_running && current_mode == Mode::DETECTION) {
            if (!detection_module.processFrame()) {
                std::cout << "目标检测模式已停止" << std::endl;
                break;
            }
        }
    }
    
    void run() {
        if (!initialize()) {
            std::cout << "系统初始化失败，程序退出!" << std::endl;
            return;
        }
        
        printMenu();
        
        while (system_running) {
            char input;
            std::cin >> input;
            
            switch (input) {
                case 't':
                case 'T':
                    current_mode = Mode::THERMAL;
                    runThermalMode();
                    if (system_running) printMenu();
                    break;
                    
                case 'd':
                case 'D':
                    current_mode = Mode::DETECTION;
                    runDetectionMode();
                    if (system_running) printMenu();
                    break;
                    
                case 'q':
                case 'Q':
                    system_running = false;
                    std::cout << "正在退出系统..." << std::endl;
                    break;
                    
                case 'h':
                case 'H':
                    printMenu();
                    break;
                    
                default:
                    std::cout << "无效指令，请重新输入: ";
                    break;
            }
        }
        
        cleanup();
    }
    
    void cleanup() {
        std::cout << "正在清理系统资源..." << std::endl;
        thermal_module.cleanup();
        detection_module.cleanup();
        cv::destroyAllWindows();
        std::cout << "系统已安全退出!" << std::endl;
    }
};

int main() {
    FireDetectionSystem system;
    system.run();
    return 0;
}
