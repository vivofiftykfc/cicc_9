/*
 * Copyright : (C) 2022 Phytium Information Technology, Inc.
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
 * FilePath: slaver_00_example.c
 * Date: 2022-03-08 22:00:15
 * LastEditTime: 2024-02-27 17:08:19
 * Description:  This is a sample demonstration application that showcases usage of rpmsg
 *  This application is meant to run on the remote CPU running baremetal code.
 *  This application echoes back data that was sent to it by the master core.
 *
 * Modify History:
 *  Ver   Who        Date         Changes
 * ----- ------     --------    --------------------------------------
 * 1.0   huanghe    2022/06/20      first release
 * 1.1   liusm      2024/02/27      fix bug
 * 1.2   liusm      2024/06/07      update for new rpmsg framework
 * 1.3   liusm      2025/02/28      update for start/stop framework
 */

/***************************** Include Files *********************************/

#include <stdio.h>
#include <fgpio.h>
#include <openamp/open_amp.h>
#include <metal/alloc.h>
#include "platform_info.h"
#include "rpmsg_service.h"
#include <metal/sleep.h>
#include "rsc_table.h"
#include "fcache.h"
#include "fdebug.h"
#include "fpsci.h"
#include "platform_info.h"
#include "helper.h"
#include "rpmsg_service.h"
#include "openamp_configs.h"
#include "fpl011.h"
#include "rsc_table.h"
#include "libmetal_configs.h"
#include <stdint.h> 
#include "slaver_00_example.h"
/************************** Constant Definitions *****************************/
#define     SLAVE_DEBUG_TAG "    SLAVE_00"
#define     SLAVE_DEBUG_I(format, ...) FT_DEBUG_PRINT_I( SLAVE_DEBUG_TAG, format, ##__VA_ARGS__)
#define     SLAVE_DEBUG_W(format, ...) FT_DEBUG_PRINT_W( SLAVE_DEBUG_TAG, format, ##__VA_ARGS__)
#define     SLAVE_DEBUG_E(format, ...) FT_DEBUG_PRINT_E( SLAVE_DEBUG_TAG, format, ##__VA_ARGS__)

#define MAX_DATA_LENGTH       (RPMSG_BUFFER_SIZE / 2)

#define DEVICE_CORE_START     0x0001U /* 开始任务 */
#define DEVICE_CORE_SHUTDOWN  0x0002U /* 关闭核心 */
#define DEVICE_CORE_CHECK     0x0003U /* 检查消息 */
/************************** Variable Definitions *****************************/
static volatile int shutdown_req = 0;

/*******************例程全局变量***********************************************/
struct remoteproc remoteproc_slave_00;
static struct rpmsg_device *rpdev_slave_00 = NULL;

/* 协议数据结构 */
typedef struct {
    uint32_t command; /* 命令字，占4个字节 */
    uint16_t length;  /* 数据长度，占2个字节 */
    char data[MAX_DATA_LENGTH];       /* 数据内容，动态长度 */
} ProtocolData;

static ProtocolData protocol_data;

/************************** 资源表定义，与linux协商一致 **********/
static struct remote_resource_table __resource resources __attribute__((used)) = {
	/* Version */
	1,

	/* NUmber of table entries */
	NUM_TABLE_ENTRIES,
	/* reserved fields */
	{0, 0,},

	/* Offsets of rsc entries */
	{
	 offsetof(struct remote_resource_table, rpmsg_vdev),
	},

	/* Virtio device entry */
	{
	 RSC_VDEV, VIRTIO_ID_RPMSG_, VDEV_NOTIFYID, RPMSG_IPU_C0_FEATURES, 0, 0, 0,
	 NUM_VRINGS, {0, 0},
	},
    
	/* Vring rsc entry - part of vdev rsc entry */
	{SLAVE00_TX_VRING_ADDR, VRING_ALIGN, SLAVE00_VRING_NUM, 1, 0},
	{SLAVE00_RX_VRING_ADDR, VRING_ALIGN, SLAVE00_VRING_NUM, 2, 0},
};

/********** 共享内存定义，与linux协商一致 **********/
static metal_phys_addr_t poll_phys_addr = SLAVE00_KICK_IO_ADDR;
struct metal_device kick_driver_00 = {
    .name = SLAVE_00_KICK_DEV_NAME,
	.bus = NULL,
    .num_regions = 1,
	.regions = {
		{
			.virt = (void *)SLAVE00_KICK_IO_ADDR,
			.physmap = &poll_phys_addr,
			.size = 0x1000,
			.page_shift = -1UL,
			.page_mask = -1UL,
			.mem_flags = SLAVE00_SOURCE_TABLE_ATTRIBUTE,
			.ops = {NULL},
		}
	},
    .irq_num = 1,/* Number of IRQs per device */
	.irq_info = (void *)SLAVE_00_SGI,
} ;

struct remoteproc_priv slave_00_priv = {
    .kick_dev_name =           SLAVE_00_KICK_DEV_NAME  ,
	.kick_dev_bus_name =        KICK_BUS_NAME ,
    .cpu_id        =  MASTER_CORE_MASK,/* 给所有core发送中断 */

	.src_table_attribute = SLAVE00_SOURCE_TABLE_ATTRIBUTE ,
	
	/* |rx vring|tx vring|share buffer| */
	.share_mem_va = SLAVE00_SHARE_MEM_ADDR ,
	.share_mem_pa = SLAVE00_SHARE_MEM_ADDR ,
	.share_buffer_offset = SLAVE00_VRING_SIZE ,
	.share_mem_size = SLAVE00_SHARE_MEM_SIZE ,
	.share_mem_attribute = SLAVE00_SHARE_BUFFER_ATTRIBUTE
} ;

/* 以下ydzhw新增MB1043传感器相关静态变量 */
static FPl011 uart_inst;
static int mb1043_initialized = 0;
/* 以上ydzhw新增MB1043传感器相关静态变量 */
// 假设UART2用于MB1043（根据硬件手册调整）
// static const FPl011Config mb1043_uart_config = {
//     .instance_id = 2,                    // UART实例ID（假设为2）
//     .base_address = 0x2800B000,          // UART2基地址（示例值，需按实际填写）
//     .ref_clock_hz = 24000000,            // 参考时钟频率（24MHz）
//     .irq_num = 45,                       // 中断号（示例值，需按实际填写）
//     .baudrate = MB1043_BAUD_RATE         // 波特率9600
// };

/************************** Function Prototypes ******************************/



/* 以下yd、zhw新增MB1043相关静态函数 */
static int mb1043_read_distance(float *distance)
{
    FError ret;
    char buffer[10] = {0};
    u32 bytes_read = 0;
    u32 timeout = MB1043_TIMEOUT_MS;
    u32 id = MB1043_UART_ID; // 假设UART2用于MB1043（根据硬件手册调整）

    /* 初始化UART（仅在首次调用时执行） */
    if (!mb1043_initialized) 
    {
        /* 配置UART参数 */
        FPl011Config config_value;
        const FPl011Config *config_p;
        FError ret;
        config_p = FPl011LookupConfig(id);
        if (NULL == config_p)
        {
            printf("Lookup ID is error!\n");
            return ERR_GENERAL;
        }
        memcpy(&config_value, config_p, sizeof(FPl011Config)) ;
        /* 设置通信格式 */
        FPl011Format format = {
            .baudrate = MB1043_BAUD_RATE,
            .data_bits = FPL011_FORMAT_WORDLENGTH_8BIT,
            .parity = FPL011_FORMAT_NO_PARITY,
            .stopbits = FPL011_FORMAT_1_STOP_BIT
        };
        memset(&uart_inst, 0, sizeof(FPl011));

        /* 初始化UART硬件 */
        ret = FPl011CfgInitialize(&uart_inst, &config_value);
        if (ret != FT_SUCCESS) 
        {
            SLAVE_DEBUG_E("MB1043 UART init failed");
            return -1;
        }

        FPl011SetDataFormat(&uart_inst, &format);

        /* 使能UART收发 */
        FPl011SetOptions(&uart_inst, 
            FPL011_OPTION_UARTEN | 
            FPL011_OPTION_FIFOEN | 
            FPL011_OPTION_RXEN | 
            FPL011_OPTION_TXEN | 
            FPL011_OPTION_DTR | 
            FPL011_OPTION_RTS
        );
        printf("FSerialPollInit baudrate:%d,data_bits:%d,parity:%d,stopbits:%d is ok.\n", 
       format.baudrate, format.data_bits + 5, format.parity, format.stopbits + 1);
        mb1043_initialized = 1;
    }

    /* 轮询读取数据 */
    while (bytes_read < sizeof(buffer) - 1) 
    {
        u32 recv_cnt = FPl011Receive(&uart_inst, &buffer[bytes_read], 1);
        if (recv_cnt > 0) 
        {
            bytes_read += recv_cnt;
            if (buffer[bytes_read-1] == '\r') 
                break; // 检测到结束符
        }

        /* 超时处理 */
        if (--timeout == 0) 
        {
            SLAVE_DEBUG_E("MB1043 read timeout");
            break;
        }
        fsleep_millisec(1);
    }

    /* 数据校验 */
    buffer[bytes_read] = '\0';
    printf("Raw data: %s\n", buffer);

    /* 解析距离值 */
    *distance = (buffer[1]-'0')*1000 + (buffer[2]-'0')*100 + (buffer[3]-'0')*10 + (buffer[4]-'0');
    *distance /= 10.0f;

    printf("Parsed distance: %.3f\n", *distance);

    return 0;
}
/* 以上yd、zhw新增MB1043相关静态函数 */
/*协议解析接口*/
int parse_protocol_data(const char* input, size_t input_size, ProtocolData* output) 
{
    int i = 0;

    if (input_size < 6) { /* 确保最小长度（命令字+数据长度）*/
        return -1; /* 数据太短 */
    }

    /* 提取命令字 */
    output->command = *((uint32_t*)input);
    input += 4;

    /* 提取数据长度 */
    output->length = *((uint16_t*)input);
    input += 2;

    /* 检查数据长度是否超出预定义最大长度 */
    if (output->length > MAX_DATA_LENGTH) {
        return -2; // 数据长度超出限制
    }

    /* 复制数据内容 */
    memcpy(output->data, input, output->length);

    return 0; /* 解析成功 */
}

/*协议组装接口*/
int assemble_protocol_data(const ProtocolData* input, char* output, size_t* output_size) 
{
    /* 检查预期的输出大小是否超出最大长度 */
    if (6 + input->length > MAX_DATA_LENGTH) {
        return -1; /* 数据长度超出限制 */
    }

    *output_size = 6 + input->length; /* 命令字+长度+数据 */

    /* 组装命令字 */
    *((uint32_t*)output) = input->command;

    /* 组装数据长度 */
    *((uint16_t*)(output + 4)) = input->length;

    /* 复制数据内容 */
    memcpy(output + 6, input->data, input->length);

    return 0; /* 组装成功 */
}

/*-----------------------------------------------------------------------------*
 *  RPMSG endpoint callbacks
 *-----------------------------------------------------------------------------*/
static int rpmsg_endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv)
{
    (void)priv;
    (void)src;

    int ret;
    int i = 0;
    (void)priv;
    SLAVE_DEBUG_I("src:0x%x",src);
    ept->dest_addr = src;

    ret = parse_protocol_data((char *)data, len, &protocol_data);
    if(ret != 0)
    {
        SLAVE_DEBUG_E("parse protocol data error,ret:%d",ret);
        return RPMSG_SUCCESS;/* 解析失败，忽略数据 */
    }
    SLAVE_DEBUG_I("command:0x%x,length:%d.",protocol_data.command,protocol_data.length);
    switch (protocol_data.command)
    {
        case DEVICE_CORE_START:
        {
            break;
        }
        case DEVICE_CORE_SHUTDOWN:
        {
            shutdown_req = 1;
            break;
        }
        case DEVICE_CORE_CHECK:
        {
            /* Send temp_data back to master */
            /* 请勿直接对data指针对应的内存进行写操作，操作vring中remoteproc发送通道分配的内存，引发错误的问题*/
            ret = rpmsg_send(ept, &protocol_data, len);
            if (ret < 0)
            {
                SLAVE_DEBUG_E("rpmsg_send failed.\r\n");
                return ret;
            }
            break;
        }
	 /* 以下yd新增距离读取case */
        case DEVICE_CORE_GET_DISTANCE:
        {
            struct {
                uint32_t command;
                uint16_t length;
                uint8_t status;
                float distance;
                uint8_t reserved;
            } response = {
                .command = DEVICE_CORE_GET_DISTANCE,
                .length = 6,
                .reserved = 0
            };
            
            response.status = mb1043_read_distance(&response.distance);
            ret = rpmsg_send(ept, &response, sizeof(response));
            if (ret < 0) {
                SLAVE_DEBUG_E("Send distance failed");
            }
            break;
        }
	 /* 以上yd新增距离读取case */
        default:
            break;
    }

    return RPMSG_SUCCESS;
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
    (void)ept;
    SLAVE_DEBUG_I("Unexpected remote endpoint destroy.\r\n");
    shutdown_req = 1;
}

/*-----------------------------------------------------------------------------*
 *  Application
 *-----------------------------------------------------------------------------*/
static int FRpmsgEchoApp(struct rpmsg_device *rdev, void *priv)
{
    int ret = 0;
    struct rpmsg_endpoint lept;
    shutdown_req = 0;
    /* Initialize RPMSG framework */
    SLAVE_DEBUG_I("Try to create rpmsg endpoint.\r\n");

    ret = rpmsg_create_ept(&lept, rdev, RPMSG_SERVICE_NAME, 0, RPMSG_ADDR_ANY, rpmsg_endpoint_cb, rpmsg_service_unbind);
    if (ret)
    {
        SLAVE_DEBUG_E("Failed to create endpoint. %d \r\n", ret);
        return -1;
    }

    SLAVE_DEBUG_I("Successfully created rpmsg endpoint.\r\n");

    while (1)
    {
        platform_poll(priv);
        /* we got a shutdown request, exit */
        if (shutdown_req || rproc_get_stop_flag())
        {
	        rproc_clear_stop_flag();
            break;
        }
    }

    rpmsg_destroy_ept(&lept);

    return ret;
}

/*-----------------------------------------------------------------------------*
 *  Application entry point
 *-----------------------------------------------------------------------------*/
int slave_init(void)
{
    init_system();  // Initialize the system resources and environment
    
    if (!platform_create_proc(&remoteproc_slave_00, &slave_00_priv, &kick_driver_00)) 
    {
        SLAVE_DEBUG_E("Failed to create remoteproc instance for slave 00\r\n");
        return -1;  // Return with an error if creation fails
    }
    
    remoteproc_slave_00.rsc_table = &resources;

    if (platform_setup_src_table(&remoteproc_slave_00,remoteproc_slave_00.rsc_table)) 
    {
        SLAVE_DEBUG_E("Failed to setup src table for slave 00\r\n");
        return -1;
    }
    
    SLAVE_DEBUG_I("Setup resource tables for the created remoteproc instances is over \r\n");

    if (platform_setup_share_mems(&remoteproc_slave_00)) 
    {
        SLAVE_DEBUG_E("Failed to setup shared memory for slave 00\r\n");
        return -1; 
    }

    SLAVE_DEBUG_I("Setup shared memory regions for both remoteproc instances is over \r\n");

    rpdev_slave_00 = platform_create_rpmsg_vdev(&remoteproc_slave_00, 0, VIRTIO_DEV_SLAVE, NULL, NULL);
    if (!rpdev_slave_00) 
    {
        SLAVE_DEBUG_E("Failed to create rpmsg vdev for slave 00\r\n");
        return -1;  /* Return with an error if creation fails */
    }

    return 0 ;   
}

static FError FSerialPollDeinit(FPl011 *uart_p)
{
    FError ret;
    FASSERT(uart_p != NULL);

    /* stop UART */
    u32 reg = 0U;
    reg &= ~(FPL011_OPTION_UARTEN | FPL011_OPTION_FIFOEN | FPL011_OPTION_RXEN | FPL011_OPTION_TXEN | FPL011_OPTION_DTR | FPL011_OPTION_RTS);
    FPl011SetOptions(uart_p, reg);

    /* set is_ready to no_ready */
    uart_p->is_ready = 0U;
    printf("FSerialPollDeinit is ok.\r\n");
    return FT_SUCCESS;
}

int slave00_rpmsg_echo_process(void)
{
    int ret = 0;
    SLAVE_DEBUG_I("Starting application...");
    if(!slave_init())
    {
        FRpmsgEchoApp(rpdev_slave_00,&remoteproc_slave_00) ;
        if (ret)
        {
            SLAVE_DEBUG_E("Failed to running echoapp");
            return platform_cleanup(&remoteproc_slave_00);
        }
        platform_release_rpmsg_vdev(rpdev_slave_00, &remoteproc_slave_00);
        SLAVE_DEBUG_I("Stopping application...");
        platform_cleanup(&remoteproc_slave_00);
    }
    else
    {
        platform_cleanup(&remoteproc_slave_00);
        SLAVE_DEBUG_E("Failed to init.\r\n");
    }
    FSerialPollDeinit(&uart_inst);
    mb1043_initialized = 0;
    SLAVE_DEBUG_I("Slave shutdown.\r\n");
    FPsciCpuOff();
    return 0 ;
}