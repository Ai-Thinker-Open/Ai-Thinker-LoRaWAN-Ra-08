/*
 * @Author: your name
 * @Date: 2022-04-22 15:36:29
 * @LastEditTime: 2022-04-26 10:12:00
 * @LastEditors: Please set LastEditors
 * @Description: 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * @FilePath: \ASR6601_AT_LoRaWAN\projects\ASR6601CB-EVAL\examples\my_example\lora_myself_net\components\lora_driver\include\lora_driver.h
 */
#ifndef __LORA_NET_H
#define __LORA_NET_H

#include <stdbool.h>
#include <stdint.h>
#include "tremo_regs.h"

int Ra08KitLoraTestStart(void);

void transmitPackets(unsigned char *sendBuf, unsigned char len);

unsigned char receivePackets(unsigned char *buffer);

void handler_uart_data(uint8_t data);

#endif
