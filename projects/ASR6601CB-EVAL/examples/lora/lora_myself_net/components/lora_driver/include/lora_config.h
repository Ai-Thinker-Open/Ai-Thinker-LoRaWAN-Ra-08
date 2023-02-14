/*
 * @Author: your name
 * @Date: 2022-04-22 15:53:39
 * @LastEditTime: 2022-04-26 16:57:14
 * @LastEditors: Please set LastEditors
 * @Description: 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * @FilePath: \ASR6601_AT_LoRaWAN\projects\ASR6601CB-EVAL\examples\my_example\lora_myself_net\components\lora_driver\include\lora_config.h
 */
#ifndef __LORA_CONFIG_H
#define __LORA_CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "tremo_gpio.h"

#define CONFIG_GATEWAY (1)

#define CONFIG_LORA_RFSW_CTRL_GPIOX GPIOD
#define CONFIG_LORA_RFSW_CTRL_PIN GPIO_PIN_11

#define CONFIG_LORA_RFSW_VDD_GPIOX GPIOA
#define CONFIG_LORA_RFSW_VDD_PIN GPIO_PIN_10

#ifdef __cplusplus
}
#endif

#endif /* __LORA_CONFIG_H */
