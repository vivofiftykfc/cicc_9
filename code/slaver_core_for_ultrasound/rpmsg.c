/*
 * Phytium's Remote Processor Control Driver
 *
 * Copyright (C) 2022 Phytium Technology Co., Ltd.
 * Author: Your Name <your.email@example.com>
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <poll.h>
#include <linux/rpmsg.h>
#include <errno.h>

/* MAVLink 命令字定义（需与从核一致） */
#define DEVICE_CORE_GET_DISTANCE 0x0004U

/* 决策阈值定义 */
#define DANGER_DISTANCE     50    /* 危险距离(急停) */
#define WARNING_DISTANCE    100   /* 警告距离(减速) */

/* 决策结果定义 */
#define DECISION_SAFE       0     /* 安全距离 */
#define DECISION_SLOW_DOWN  1     /* 需要减速 */
#define DECISION_STOP       2     /* 需要急停 */
#define DECISION_ERROR      3     /* 传感器错误 */

/* RPMSG 缓冲区大小（固定256字节） */
#define RPMSG_BUFFER_SIZE   256

/* 响应数据结构（填充至256字节） */
typedef struct __attribute__((packed)) {
    uint32_t command;      // 4字节
    uint16_t length;       // 2字节
    uint8_t status;        // 1字节
    float distance;        // 4字节
    uint8_t decision;      // 1字节
    uint8_t padding[RPMSG_BUFFER_SIZE - 12]; // 填充剩余244字节
} DistanceResponse;

/* 请求数据结构（填充至256字节） */
typedef struct __attribute__((packed)) {
    uint32_t command;      // 4字节
    uint16_t length;       // 2字节
    uint8_t padding[RPMSG_BUFFER_SIZE - 6]; // 填充剩余250字节
} DistanceRequest;

/* 安全读写函数 */
static ssize_t write_full(int fd, void *buf, size_t count) {
    ssize_t ret, total = 0;
    while (count) {
        ret = write(fd, buf, count);
        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }
        count -= ret;
        buf += ret;
        total += ret;
    }
    return total;
}

static ssize_t read_full(int fd, void *buf, size_t count) {
    ssize_t res;
    do {
        res = read(fd, buf, count);
    } while (res < 0 && errno == EINTR);
    return res;
}

int main(int argc, char **argv) {
    int ctrl_fd, rpmsg_fd, ret;
    struct rpmsg_endpoint_info eptinfo;
    struct pollfd fds;

    /* 1. 打开RPMSG控制设备 */
    ctrl_fd = open("/dev/rpmsg_ctrl0", O_RDWR);
    if (ctrl_fd < 0) {
        perror("open rpmsg_ctrl0 failed");
        return -1;
    }

    /* 2. 创建通信端点 */
    strncpy(eptinfo.name, "mavlink_endpoint", sizeof(eptinfo.name));
    eptinfo.src = 0;
    eptinfo.dst = 0;

    ret = ioctl(ctrl_fd, RPMSG_CREATE_EPT_IOCTL, &eptinfo);
    if (ret < 0) {
        perror("ioctl create endpoint failed");
        goto err_close_ctrl;
    }

    /* 3. 打开RPMSG通信设备 */
    rpmsg_fd = open("/dev/rpmsg0", O_RDWR);
    if (rpmsg_fd < 0) {
        perror("open rpmsg0 failed");
        goto err_close_ctrl;
    }

    /* 4. 配置poll监听 */
    memset(&fds, 0, sizeof(fds));
    fds.fd = rpmsg_fd;
    fds.events = POLLIN;

    /* 5. 主循环：发送请求并接收响应 */
    while (1) {
        /* 发送256字节请求（填充结构体） */
        DistanceRequest req = {
            .command = DEVICE_CORE_GET_DISTANCE,
            .length = 0
        };
        memset(req.padding, 0, sizeof(req.padding)); // 填充清零

        ret = write_full(rpmsg_fd, &req, sizeof(req));
        if (ret != sizeof(req)) {
            perror("write_full failed");
            break;
        }

        /* 等待响应（超时20ms） */
        ret = poll(&fds, 1, 20);
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("poll failed");
            break;
        }

        if (ret == 0) {
            printf("poll timeout, retrying...\n");
            continue;
        }

        /* 接收256字节响应 */
        DistanceResponse resp;
        ret = read_full(rpmsg_fd, &resp, sizeof(resp));
        if (ret != sizeof(resp)) {
            perror("read_full failed");
            break;
        }

        /* 解析响应数据 */
        if (resp.command == DEVICE_CORE_GET_DISTANCE) {
            if (resp.status == 0) {
                
                if (resp.distance < 0) {
                    continue;
                }
                
                printf("Distance: %.1f cm - ", resp.distance);
                switch (resp.decision) {
                    case DECISION_SAFE:     printf("SAFE\n"); break;
                    case DECISION_SLOW_DOWN: printf("WARNING: SLOW DOWN\n"); break;
                    case DECISION_STOP:    printf("DANGER: STOP\n"); break;
                    default:              printf("UNKNOWN DECISION\n");
                }
            } else {
                printf("Sensor error: %d\n", resp.status);
            }
        }

        usleep(100000); // 100ms间隔
    }

    /* 6. 清理资源 */
    close(rpmsg_fd);
err_close_ctrl:
    close(ctrl_fd);
    return 0;
}