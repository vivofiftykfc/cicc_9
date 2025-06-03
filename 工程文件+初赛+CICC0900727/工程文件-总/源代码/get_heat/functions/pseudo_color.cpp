#include "pseudo_color.h"
#include <cmath>

#define abs(x) ((x)>0?(x):-(x))

// 这里只实现一种伪彩色方法（可扩展）
void GrayToPseColor(uint8_t method, uint8_t grayValue, uint8_t &colorR, uint8_t &colorG, uint8_t &colorB)
{
    switch(method) {
        case 0: // Pseudo1
            colorR = abs(0 - grayValue);
            colorG = abs(127 - grayValue);
            colorB = abs(255 - grayValue);
            break;
        default:
            colorR = grayValue;
            colorG = grayValue;
            colorB = grayValue;
            break;
    }
}
