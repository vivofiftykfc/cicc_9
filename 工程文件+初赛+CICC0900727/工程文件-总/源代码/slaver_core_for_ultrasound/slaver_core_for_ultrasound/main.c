/*
 * @ : Copyright (c) 2021 Phytium Information Technology, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0.
 *
 * @Date: 2021-12-09 17:12:52
 * LastEditTime: 2023-01-11 15:36:09
 * @Description:  This file is for openamp test
 *
 * @Modify History:
 *  Ver   Who           Date         Changes
 * ----- ------         --------    --------------------------------------
 * 1.0   huanghe        2022/06/20      first release
 * 1.1  liushengming    2023/05/16      for openamp test
 */
/***************************** Include Files *********************************/
#include <openamp/version.h>
#include <metal/version.h>
#include <stdio.h>
#include "ftypes.h"
#include "fsleep.h"
#include "fprintk.h"
#include "stdio.h"
#include "fdebug.h"
#include "fcache.h"
#include "sdkconfig.h"
#include "slaver_00_example.h"

/************************** Function Prototypes ******************************/

int main(void)
{
    printf("complier data: %s ,time: %s \r\n", __DATE__, __TIME__);
    printf("openamp lib version: %s (", openamp_version());
    printf("Major: %d, ", openamp_version_major());
    printf("Minor: %d, ", openamp_version_minor());
    printf("Patch: %d)\r\n", openamp_version_patch());

    printf("libmetal lib version: %s (", metal_ver());
    printf("Major: %d, ", metal_ver_major());
    printf("Minor: %d, ", metal_ver_minor());
    printf("Patch: %d)\r\n", metal_ver_patch());
    /*run the example*/
    return slave00_rpmsg_echo_process();
}

