#include <cstdio>
#include <unistd.h>                   
#include "MLX90640_I2C_Driver.h" 
#include "MLX90640_API.h"
#include <opencv2/opencv.hpp>
#include "pseudo_color.h"

#define  FPS2HZ   0x02
#define  FPS4HZ   0x03
#define  FPS8HZ   0x04
#define  FPS16HZ  0x05
#define  FPS32HZ  0x06

#define  MLX90640_ADDR 0x33
#define	 RefreshRate FPS4HZ 
#define  TA_SHIFT 8 //Default shift for MLX90640 in open air

static uint16_t eeMLX90640[832];  
static float mlx90640To[768];
static uint16_t frame[834];
float emissivity=0.95;
int status;

int main(int argc, char **argv)
{
    // 1) 初始化 I2C 驱动
	if (MLX90640_I2CInit() != 0) {
        printf("I2C Init failed\n");
        return -1;
    }
	
	// 2) 设置帧率与模式
    status = MLX90640_SetRefreshRate(MLX90640_ADDR, RefreshRate);
    if (status != 0) printf("SetRefreshRate error %d\n", status);
    status = MLX90640_SetChessMode(MLX90640_ADDR);
    if (status != 0) printf("SetChessMode error %d\n", status);

    // 3) 读取并解析校准参数
    status = MLX90640_DumpEE(MLX90640_ADDR, eeMLX90640);
    if (status != 0) {
        printf("DumpEE error %d\n", status);
        return -1;
    }
    paramsMLX90640 mlx90640;
    status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
    if (status != 0) {
        printf("ExtractParameters error %d\n", status);
        return -1;
    }

    const useconds_t delay_us = 1000000 / 4;//4Hz
    cv::namedWindow("MLX90640 Thermal", cv::WINDOW_NORMAL);
    cv::resizeWindow("MLX90640 Thermal", 800, 600);
	while (1)
	{
		int status = MLX90640_GetFrameData(MLX90640_ADDR, frame);  //读取一帧原始数据
		if (status < 0)
		{
			printf("GetFrame Error: %d\r\n",status);
            continue;
		}
		float vdd = MLX90640_GetVdd(frame, &mlx90640);
		float Ta = MLX90640_GetTa(frame, &mlx90640); 
		float tr = Ta - TA_SHIFT;

		MLX90640_CalculateTo(frame, &mlx90640, emissivity , tr, mlx90640To);            //计算像素点温度

        // 归一化温度到0~255
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
                GrayToPseColor(0, gray, r, g, b); // 0: 伪彩色方法
                img.at<cv::Vec3b>(i, j) = cv::Vec3b(b, g, r); // OpenCV为BGR
            }
        }
        // 插值放大
        cv::Mat img_big;
        cv::resize(img, img_big, cv::Size(320, 240), 0, 0, cv::INTER_CUBIC);
        cv::imshow("MLX90640 Thermal", img_big);
        cv::waitKey(1);

        usleep(delay_us);
	}
}
