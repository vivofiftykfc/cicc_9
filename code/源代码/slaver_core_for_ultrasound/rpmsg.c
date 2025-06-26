/*
 * Phytium's Remote Processor Control Driver
 *
 * Copyright (C) 2022 Phytium Technology Co., Ltd. - All Rights Reserved
 * Author: Shaojun Yang <yangshaojun@phytium.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify it under the terms 
 * of the GNU General Public License version 2 as published by the Free Software Foundation.
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
/* 命令字定义，需与从核一致 */
#define DEVICE_CORE_GET_DISTANCE 0x0004U

/* 响应数据结构 */
typedef struct {
    uint32_t command;
    uint16_t length;
    uint8_t status;
    float distance;
    uint8_t reserved;
} DistanceResponse;
static ssize_t write_full(int fd, void *buf, size_t count)
{
	ssize_t ret = 0;
	ssize_t total = 0;

	while (count) {
		ret = write(fd, buf, count);
		if (ret < 0) {
			if (errno == EINTR)
				continue;

			break;
		}

		count -= ret;
		buf += ret;
		total += ret;
	}   

	return total;
}

static ssize_t read_full(int fd, void *buf, size_t count)
{
	ssize_t res;

	do {
		res = read(fd, buf, count);
	} while (res < 0 && errno == EINTR);

	if ((res < 0 && errno == EAGAIN))
		return 0;

	if (res < 0)
		return -1; 

	return res;
}

static int running = 1, no = 0; 

int main(int argc, char **argv)
{
	int ctrl_fd, rpmsg_fd, ret;
	struct rpmsg_endpoint_info eptinfo;
	struct pollfd fds;
	uint8_t buf[32], r_buf[32];

	ctrl_fd = open("/dev/rpmsg_ctrl0", O_RDWR);
	if (ctrl_fd < 0) {
		perror("open rpmsg_ctrl0 failed.\n");
		return -1;
	}

	memcpy(eptinfo.name, "xxxx", 32);
	eptinfo.src = 0;
	eptinfo.dst = 0;

	ret = ioctl(ctrl_fd, RPMSG_CREATE_EPT_IOCTL, eptinfo);	
	if (ret != 0) {
		perror("ioctl RPMSG_CREATE_EPT_IOCTL failed.\n");
		goto err0;
	}

	rpmsg_fd = open("/dev/rpmsg0", O_RDWR);
	if (rpmsg_fd < 0) {
		perror("open rpmsg0 failed.\n");
		goto err1;
	}

	memset(&fds, 0, sizeof(struct pollfd));
	fds.fd = rpmsg_fd;
	fds.events |= POLLIN; 

	/* receive message from remote processor. */
	while (running) {
		 /* 构造距离查询请求 */
        struct {
            uint32_t command;
            uint16_t length;
        } request = {
            .command = DEVICE_CORE_GET_DISTANCE,
            .length = 0
        };

		/* 发送距离查询命令 */
        ret = write_full(rpmsg_fd, &request, 6); // 4字节命令 + 2字节长度
        if (ret < 0) {
            perror("write_full failed.");
            goto err1;
        }


		ret = poll(&fds, 1, 5);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			perror("poll failed.\n");
			break;
		}

		if (ret == 0) {
			printf("poll timeout.\n");
			continue;
		}

		memset(r_buf, 0, sizeof(r_buf));
        ret = read_full(rpmsg_fd, r_buf, sizeof(r_buf));
        if (ret < 0) {
            perror("read_full failed.");
            break;
        }

        /* 解析响应数据 */
        if (ret >= sizeof(DistanceResponse)) {
            DistanceResponse *resp = (DistanceResponse *)r_buf;
            if (resp->command == DEVICE_CORE_GET_DISTANCE) {
                if (resp->status == 0) {
                    printf("Distance: %.1f cm\n", resp->distance);
                } else {
                    printf("Sensor error: %d\n", resp->status);
                }
            }
        }

		/* output message */
		printf("received message: %s\n", r_buf);

		if (strncmp((char *)r_buf, "shutdown", 8) == 0) {
			printf("Shutdown command received. Exiting loop.\n");
			running = 0;
}

		usleep(1);
	}

err1:
	close(rpmsg_fd);
err0:
	close(ctrl_fd);

	return 0;
}
