/*
 * Copyright : (C) 2024 Phytium Information Technology, Inc.
 * All Rights Reserved.
 * 
 * This program is OPEN SOURCE software: you can redistribute it and/or modify it
 * under the terms of the Phytium Public License as published by the Phytium Technology Co.,Ltd,
 * either version 1.0 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the Phytium Public License for more details.
 * 
 * 
 * FilePath: slaver_00_example.h
 * Created Date: 2024-04-29 16:17:53
 * Last Modified: 2024-06-07 10:35:22
 * Description:  This file is for
 * 
 * Modify History:
 *  Ver      Who        Date               Changes
 * -----  ----------  --------  ---------------------------------
 */

#ifndef SLAVER_00_EXAMPLE_H
#define SLAVER_00_EXAMPLE_H

#include "ftypes.h"
#include "fsleep.h"

int slave00_rpmsg_echo_process(void);

#endif /* SLAVER_00_EXAMPLE_H */

/* 以下yd新增部分 */
#define DEVICE_CORE_GET_DISTANCE  0x0004U /* Get distance data */
#define MB1043_UART_PORT    "/dev/ttyAMA2"
#define MB1043_BAUD_RATE    9600
#define MB1043_TIMEOUT_MS   100
#define MB1043_MIN_DIST     30
#define MB1043_MAX_DIST     500
#define MB1043_UART_ID     2
/* 以上yd新增部分 */