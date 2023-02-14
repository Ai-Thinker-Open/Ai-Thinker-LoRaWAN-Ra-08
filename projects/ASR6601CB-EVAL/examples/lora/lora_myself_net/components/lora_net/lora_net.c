/*
 * @Author: your name
 * @Date: 2022-04-22 14:46:59
 * @LastEditTime: 2022-04-26 15:10:03
 * @LastEditors: Please set LastEditors
 * @Description: 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * @FilePath: \ASR6601_AT_LoRaWAN\projects\ASR6601CB-EVAL\examples\my_example\lora_myself_net\components\lora_net\lora_net.c
 */
#include "lora_net.h"
#include "lora_driver.h"
/**
 * 功能：根据ModBus规则计算CRC16
 * 参数：
 *       _pBuf:待计算数据缓冲区,计算得到的结果存入_pBuf的最后两字节
 *       _usLen:待计算数据长度(字节数)
 * 返回值：16位校验值
 */
static unsigned short int getModbusCRC16(unsigned char *_pBuf, uint16_t _usLen)
{
    unsigned short int CRCValue = 0xFFFF; // 初始化CRC变量各位为1
    unsigned char i, j;

    for (i = 0; i < _usLen; ++i)
    {
        CRCValue ^= *(_pBuf + i); // 当前数据异或CRC低字节
        for (j = 0; j < 8; ++j)   // 一个字节重复右移8次
        {
            if ((CRCValue & 0x01) == 0x01) // 判断右移前最低位是否为1
            {
                CRCValue = (CRCValue >> 1) ^ 0xA001; // 如果为1则右移并异或表达式
            }
            else
            {
                CRCValue >>= 1; // 否则直接右移一位
            }
        }
    }

    if (CRCValue & 0xFF == 0)
    {
        CRCValue |= 0xA5;
    }
    return CRCValue;
}

static unsigned char getFrameLength(unsigned char *pbuffer, unsigned char buffer_len)
{
    unsigned char len = buffer_len - 1;

    /*数据帧长度最大值不会超过255*/
    while (len)
    {
        if (*(pbuffer + len) != 0)
        {
            break;
        }
        --len;
    }
    return len + 1;
}

void sendMasterAsk(unsigned char slave_addr, unsigned char op_code, unsigned char pram)
{

    /*发送数据缓冲区，其大小根绝实际情况设置，本例程为6*/
    unsigned char sendbuffer[6] = {NET_ADDR, slave_addr, op_code, pram, 0, 0};
    unsigned short int CRC16 = getModbusCRC16(sendbuffer, 4); // 计算CRC值

    sendbuffer[4] = CRC16 >> 8; // 赋值CRC高位
    sendbuffer[5] = CRC16;      // 赋值CRC低位

    transmitPackets(sendbuffer, sizeof(sendbuffer)); // 发送数据缓冲区
}
FrameStatus receiveSlaveAck(unsigned char slave_addr, unsigned char op_code, unsigned char pram, DeviceBlock *pdevblock, unsigned char *receivebuffer, uint16_t len)
{
    /*接收数据缓冲区，其大小根绝实际情况设置，本例程最大值为9*/

    unsigned char i;

    if (receivebuffer[0] != NET_ADDR) // 不是本网络数据，扔掉
    {
        return FRAME_NETADDR_ERR;
    }

    if (receivebuffer[1] != slave_addr) // 不是目标从机发送的数据，扔掉
    {
        printf("Recieve error FRAME_SLAVEADDR_ERR ...\r\n");
        return FRAME_SLAVEADDR_ERR;
    }

    if (getModbusCRC16(receivebuffer, len - 2) != (receivebuffer[len - 2] << 8 | receivebuffer[len - 1])) // 校验值不对数据无效扔掉
    {
        printf("Recieve error FRAME_CRC_ERR ...\r\n");
        return FRAME_CRC_ERR;
    }

    /*一切正常开始处理数据*/
    if (op_code == OP_W_COILS) // 此时是写线圈(继电器)操作
    {
        (pdevblock + slave_addr)->Coils = receivebuffer[3]; //
    }
    else if (op_code == OP_R_SENSOR) // 此时是读寄存器操作
    {
        /**
         * 该函数传入的应该是一个DeviceBlock类型的数组，每个从机对应一个DeviceBlock类型的数组元素
         */
        for (i = 0; i < 8; ++i)
        {

            switch (pram & (0x01 << i))
            {
            case PRAM_R_TEMPERATURE:
                (pdevblock + slave_addr)->Temperature = receivebuffer[3];
                break;
            case PRAM_R_HUMIDITY:
                (pdevblock + slave_addr)->Humidity = receivebuffer[4];
                break;
            case PRAM_R_LUX:
                (pdevblock + slave_addr)->Lux = receivebuffer[5] << 8 | receivebuffer[6];
                break;

            default:
                break;
            }
        }
    }
    else
    {
        /*其他操作码可在此扩充*/
    }
    return FRAME_OK; // 接收数据成功
}

FrameStatus processMasterAsk(DeviceBlock *pdevblock, unsigned char *Askbuffer, uint16_t len)
{
    /*
    unsigned char Ackbuffer[9] = {NET_ADDR, SLAVE1_ADDR};
    unsigned short int CRC16;
    unsigned char i;

    if (Askbuffer[0] != NET_ADDR) // 不是本网络数据，扔掉
    {

        return FRAME_NETADDR_ERR;
    }

    if (Askbuffer[1] != SLAVE1_ADDR) // 不是目标从机发送的数据，扔掉
    {
        printf("Recieve control:%d , local:%d ...\r\n", Askbuffer[1], SLAVE1_ADDR);
        return FRAME_SLAVEADDR_ERR;
    }

    if (getModbusCRC16(Askbuffer, len - 2) != (Askbuffer[len - 2] << 8 | Askbuffer[len - 1])) // 校验值不对数据无效扔掉
    {

        printf("Recieve FRAME_CRC_ERR \r\n");
        return FRAME_CRC_ERR;
    }

    printf("Recieve Askbuffer[2]: %d \r\n", Askbuffer[2]);

    if (Askbuffer[2] == OP_W_COILS)
    {
        Ackbuffer[2] = OP_W_COILS;
        pdevblock->Coils = Askbuffer[3];
        Ackbuffer[3] = pdevblock->Coils;
        CRC16 = getModbusCRC16(Ackbuffer, 4);
        Ackbuffer[4] = CRC16 >> 8;
        Ackbuffer[5] = CRC16;
    }
    else if (Askbuffer[2] == OP_R_SENSOR)
    {
        Ackbuffer[2] = OP_R_SENSOR;
        for (i = 0; i < 8; ++i)
        {
            switch (Askbuffer[3] & (0x01 << i))
            {
            case PRAM_R_TEMPERATURE:
                Ackbuffer[3] = pdevblock->Temperature;
                break;
            case PRAM_R_HUMIDITY:
                Ackbuffer[4] = pdevblock->Humidity;
                break;
            case PRAM_R_LUX:
            {
                Ackbuffer[5] = pdevblock->Lux >> 8;
                Ackbuffer[6] = pdevblock->Lux;
            }
            break;

            default:
                break;
            }
        }

        CRC16 = getModbusCRC16(Ackbuffer, 7);
        Ackbuffer[7] = CRC16 >> 8;
        Ackbuffer[8] = CRC16;
    }
    else
    {
        /*其他操作码可在此扩充
}

transmitPackets(Ackbuffer, sizeof(Ackbuffer));
*/
    return FRAME_OK;
}
