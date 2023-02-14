/*
 * @Author: your name
 * @Date: 2022-04-22 14:47:09
 * @LastEditTime: 2022-05-06 20:10:18
 * @LastEditors: xuhongv@yeah.net xuhongv@yeah.net
 * @Description: 打开koroFileHeader查看配置 进�?��?�置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * @FilePath: \ASR6601_AT_LoRaWAN\projects\ASR6601CB-EVAL\examples\my_example\lora_myself_net\components\lora_net\include\lora_net.h
 */
#ifndef __LORA_DRIVER_H
#define __LORA_DRIVER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tremo_regs.h"
#include "lora_config.h"

/*网络相关宏定�?*/
#define NET_ADDR 0xA5       // 网络地址
#define BROADCAST_ADDR 0xFF // 广播地址

/*操作码相关宏定义*/
#define OP_W_COILS 0x02  // 写继电器状�?
#define OP_R_SENSOR 0x01 // 读传感器数据

/*参数相关宏定�?*/
#define PRAM_R_TEMPERATURE 0x01 // �?读取温度
#define PRAM_R_HUMIDITY 0x02    // �?读取湿度
#define PRAM_R_LUX 0x04         // �?读取光线强度
#define PRAM_R_ALL 0x07         // 读取所有传感器

#define PRAM_W_RELAY1 0x01 // 吸合继电�?1,如果想断开按位取反即可
#define PRAM_W_RELAY2 0x02 // 吸合继电�?2,如果想断开按位取反即可

typedef enum
{
    FRAME_OK = 0x00,            // 数据帧�?�确
    FRAME_NETADDR_ERR = 0x01,   // 网络地址错�??
    FRAME_SLAVEADDR_ERR = 0x02, // 从机地址错�??
    FRAME_CRC_ERR = 0x03,       // CRC校验错�??
    FRAME_EMPTY = 0xFF          // 数据为空，�?�时没接到数�?
} FrameStatus;

typedef struct
{
    /**
     * 对于主机�?来�?�，还可以在最前面添加各个从机的运行状�?
     * 如：unsigned char SlaveStatus[256];
     * 每个数组成员都�?�应一�?从机的状态，比�?�电量不足、通信异常等等
     * 这里没有添加
     * */
    unsigned char Coils;       // 线圈状�?,Bit0对应继电�?1,Bit1对应继电�?2
    unsigned char Temperature; // �?境温�?
    unsigned char Humidity;    // �?境湿�?
    unsigned short int Lux;    // �?境光照强�?
    /*�?继续添加其他传感器、�??控单元和系统参数*/
} DeviceBlock;

typedef struct
{
    u8 Humidity;
    u8 Temperature;
    u16 Lux;
} DeviceSta_Strcture;

void sendMasterAsk(unsigned char slave_addr, unsigned char op_code, unsigned char pram);
FrameStatus receiveSlaveAck(unsigned char slave_addr, unsigned char op_code, unsigned char pram, DeviceBlock *pdevblock, unsigned char *receivebuffer, uint16_t len);
FrameStatus processMasterAsk(DeviceBlock *pdevblock, unsigned char *receivebuffer, uint16_t len);

#endif
