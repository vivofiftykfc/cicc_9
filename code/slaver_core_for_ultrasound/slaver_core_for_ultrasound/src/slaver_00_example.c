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
// Debug宏定义，便于统一调试输出
#define     SLAVE_DEBUG_TAG "    SLAVE_00"
#define     SLAVE_DEBUG_I(format, ...) FT_DEBUG_PRINT_I( SLAVE_DEBUG_TAG, format, ##__VA_ARGS__)
#define     SLAVE_DEBUG_W(format, ...) FT_DEBUG_PRINT_W( SLAVE_DEBUG_TAG, format, ##__VA_ARGS__)
#define     SLAVE_DEBUG_E(format, ...) FT_DEBUG_PRINT_E( SLAVE_DEBUG_TAG, format, ##__VA_ARGS__)

// MAX_DATA_LENGTH: 定义协议数据最大长度（RPMSG缓冲区一半），用于协议数据收发
#define MAX_DATA_LENGTH       (RPMSG_BUFFER_SIZE / 2)

// 协议命令字定义，主从通信时用于区分不同操作
#define DEVICE_CORE_START     0x0001U /* 开始任务 */
#define DEVICE_CORE_SHUTDOWN  0x0002U /* 关闭核心 */
#define DEVICE_CORE_CHECK     0x0003U /* 检查消息 */
// DEVICE_CORE_GET_DISTANCE 由用户自定义，读取距离

/************************** Variable Definitions *****************************/
// shutdown_req: 关机请求标志，主控下发DEVICE_CORE_SHUTDOWN命令时置1
static volatile int shutdown_req = 0;

/*******************例程全局变量***********************************************/
// remoteproc_slave_00: 本核remoteproc实例，代表本核的处理器资源
struct remoteproc remoteproc_slave_00;
// rpdev_slave_00: 本核RPMSG设备指针，用于主从消息通信
static struct rpmsg_device *rpdev_slave_00 = NULL;

/* 协议数据结构体，包含命令字、数据长度、数据内容 */
typedef struct {
    uint32_t command; /* 命令字，占4个字节 */
    uint16_t length;  /* 数据长度，占2个字节 */
    char data[MAX_DATA_LENGTH];       /* 数据内容，动态长度 */
} ProtocolData;

// protocol_data: 用于存放当前解析的协议数据
static ProtocolData protocol_data;

/************************** 资源表定义，与linux协商一致 **********/
// resources: 远程处理器资源表，描述本核与主核协商的共享内存、vring等信息
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
// poll_phys_addr: 共享内存物理地址
static metal_phys_addr_t poll_phys_addr = SLAVE00_KICK_IO_ADDR;
// kick_driver_00: 共享内存设备描述，包含中断、内存等信息
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

// slave_00_priv: remoteproc私有参数，描述kick、共享内存等
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
// uart_inst: MB1043传感器所用UART实例
static FPl011 uart_inst;
// mb1043_initialized: MB1043 UART初始化标志
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
/*
 * @brief  读取MB1043超声波传感器距离值，首次调用时自动初始化UART
 * @param  distance [out] 解析得到的距离值（单位：cm）
 * @return 0成功，负值失败
 * 逻辑说明：
 *   - 首次调用时初始化UART，设置波特率、数据位、校验位、停止位
 *   - 轮询读取串口数据，遇到回车符结束，超时则报错
 *   - 解析原始数据为距离值
 */
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
        //这几个函数用的都是他官方sdk包里面给的初始化函数，那些FP开头的
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

    /* 轮询读取数据，直到遇到回车或超时 */
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

    /* 数据校验与解析 */
    buffer[bytes_read] = '\0';
    printf("Raw data: %s\n", buffer);

    /* 解析距离值，假设格式为Rxxxx\r */
    *distance = (buffer[1]-'0')*1000 + (buffer[2]-'0')*100 + (buffer[3]-'0')*10 + (buffer[4]-'0');
    *distance /= 10.0f;

    printf("Parsed distance: %.3f\n", *distance);

    return 0;
}
/* 以上yd、zhw新增MB1043相关静态函数 */
/*
 * @brief  解析主控下发的协议数据，提取命令字、数据长度和内容
 * @param  input      输入数据指针
 * @param  input_size 输入数据长度
 * @param  output     输出协议结构体
 * @return 0成功，负值失败
 * 逻辑说明：
 *   - 检查最小长度
 *   - 提取命令字、数据长度、数据内容
 *   - 检查长度合法性
 */
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

/*
 * @brief  组装协议数据，便于回传主控
 * @param  input      输入协议结构体
 * @param  output     输出数据缓冲区
 * @param  output_size 输出数据长度
 * @return 0成功，负值失败
 * 逻辑说明：
 *   - 检查长度合法性
 *   - 组装命令字、数据长度、数据内容
 */
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
/*
 * @brief  RPMSG端点回调，处理主控发来的消息
 * @param  ept   RPMSG端点
 * @param  data  收到的数据
 * @param  len   数据长度
 * @param  src   源地址
 * @param  priv  私有数据
 * @return always RPMSG_SUCCESS
 * 逻辑说明：
 *   - 解析协议数据
 *   - 根据命令字分发处理（启动、关机、检查、读取距离等）
 *   - 回应主控请求
 */
static int rpmsg_endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv)
{
    (void)priv;
    (void)src;

    int ret;
    int i = 0;
    (void)priv;
    SLAVE_DEBUG_I("src:0x%x",src);
    ept->dest_addr = src;

    // 解析协议数据
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
            // 启动命令，预留
            break;
        }
        case DEVICE_CORE_SHUTDOWN:
        {
            // 关机命令，设置关机标志
            shutdown_req = 1;
            break;
        }
        case DEVICE_CORE_CHECK:
        {
            // 检查命令，回传收到的数据
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
            // 读取MB1043距离并回传
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
            // 未知命令，忽略
            break;
    }

    return RPMSG_SUCCESS;
}

/*
 * @brief  RPMSG端点解绑回调，主控销毁端点时触发
 * @param  ept   RPMSG端点
 * 逻辑说明：
 *   - 设置关机请求标志
 */
static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
    (void)ept;
    SLAVE_DEBUG_I("Unexpected remote endpoint destroy.\r\n");
    shutdown_req = 1;
}

/*-----------------------------------------------------------------------------*
 *  Application
 *-----------------------------------------------------------------------------*/
/*
 * @brief  RPMSG主循环应用，创建端点并轮询处理消息
 * @param  rdev  RPMSG设备
 * @param  priv  私有数据
 * @return 0成功，负值失败
 * 逻辑说明：
 *   - 创建RPMSG端点
 *   - 进入主循环，轮询处理消息
 *   - 检测到关机请求或stop_flag后退出
 */
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

    // 主循环，轮询处理RPMSG消息
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

/*
 * @brief  从核应用初始化，完成remoteproc、资源表、共享内存、RPMSG设备等初始化
 * @return 0成功，负值失败
 * 逻辑说明：
 *   - 初始化系统资源
 *   - 创建remoteproc实例
 *   - 配置资源表、共享内存
 *   - 创建RPMSG虚拟设备
 */
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

/*
 * @brief  关闭串口并释放相关资源
 * @param  uart_p  UART实例指针
 * @return FT_SUCCESS
 * 逻辑说明：
 *   - 关闭UART相关功能
 *   - 清除is_ready标志
 */
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

/*
 * @brief  从核主流程入口，完成初始化、主循环、资源释放、关机等
 * @return 0
 * 逻辑说明：
 *   - 初始化从核应用
 *   - 进入RPMSG主循环
 *   - 释放资源，关闭串口，关机
 */
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