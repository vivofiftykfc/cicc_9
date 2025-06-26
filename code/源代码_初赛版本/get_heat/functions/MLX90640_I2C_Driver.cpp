/**
 * @copyright (C) 2017 Melexis N.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include "MLX90640_I2C_Driver.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/i2c.h>      // struct i2c_msg
#include <linux/i2c-dev.h>  // I2C_RDWR
#include <sys/ioctl.h>
#include <errno.h>

#define I2C_DEVICE       "/dev/i2c-2"
#define MAX_I2C_BYTES    256 

static int fd = -1;

static int MLX90640_I2CReadCombined(uint8_t addr,
    uint16_t reg,
    uint16_t cnt,
    uint16_t *data)
{
int bytes = cnt * 2;
uint8_t  out[2] = { reg >> 8, reg & 0xFF };
uint8_t *in  = (uint8_t *)malloc(bytes);
if (!in) {
perror("malloc");
return -1;
}

struct i2c_msg msgs[2] = {
{ .addr  = addr, .flags = 0,        .len = 2,     .buf = out },
{ .addr  = addr, .flags = I2C_M_RD, .len = bytes, .buf = in  }
};
struct i2c_rdwr_ioctl_data rdwr = {
.msgs  = msgs,
.nmsgs = 2
};

if (ioctl(fd, I2C_SLAVE, addr) < 0) {
perror("ioctl SLAVE");
free(in);
return -1;
}
if (ioctl(fd, I2C_RDWR, &rdwr) < 0) {
// combined 不支持或失败
free(in);
return -1;
}

// 拆包到输出数组
for (int i = 0; i < cnt; i++) {
data[i] = (in[2*i] << 8) | in[2*i + 1];
}
free(in);
return 0;
}

static int MLX90640_I2CReadChunked(uint8_t addr,
    uint16_t reg,
    uint16_t cnt,
    uint16_t *data)
{
int total  = cnt * 2;
int offset = 0;
uint8_t ptr[2];

while (offset < total) {
int chunk = total - offset;
if (chunk > MAX_I2C_BYTES) chunk = MAX_I2C_BYTES;

// 更新寄存器指针：reg + 已读 word 数
uint16_t curReg = reg + offset/2;
ptr[0] = curReg >> 8;
ptr[1] = curReg & 0xFF;
if (write(fd, ptr, 2) != 2) {
perror("write ptr");
return -1;
}

uint8_t buf[MAX_I2C_BYTES];
if (read(fd, buf, chunk) != chunk) {
perror("read chunk");
return -1;
}
for (int i = 0; i < chunk / 2; i++) {
data[offset/2 + i] = (buf[2*i] << 8) | buf[2*i + 1];
}
offset += chunk;
}
return 0;
}


// 打开 I2C 总线
int MLX90640_I2CInit(void) {
    if (fd >= 0) return 0;
    fd = open("/dev/i2c-2", O_RDWR);
    return (fd < 0) ? -1 : 0;
}

int MLX90640_I2CRead(uint8_t addr, uint16_t reg, uint16_t cnt, uint16_t *data) {
    if (fd < 0 && MLX90640_I2CInit() < 0) {
        fprintf(stderr, "I2CInit failed\n");
        return -1;
    }

    // 尝试 combined 事务
    if (MLX90640_I2CReadCombined(addr, reg, cnt, data) == 0) {
        return 0;
    }
    // 若 combined 失败，回退到分块读取
    return MLX90640_I2CReadChunked(addr, reg, cnt, data);
}

int MLX90640_I2CWrite(uint8_t addr, uint16_t reg, uint16_t val) {
    if (fd < 0 && MLX90640_I2CInit() < 0) return -1;
    if (ioctl(fd, I2C_SLAVE, addr) < 0) return -1;

    uint8_t wbuf[4] = {
        reg >> 8, reg & 0xFF,
        val >> 8, val & 0xFF
    };
    if (write(fd, wbuf, 4) != 4) return -1;
    return 0;
}
