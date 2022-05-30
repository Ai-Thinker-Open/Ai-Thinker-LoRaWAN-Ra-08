#ifndef __LORA_CONFIG_H
#define __LORA_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tremo_gpio.h"

#define CONFIG_LORA_RFSW_CTRL_GPIOX GPIOD
#define CONFIG_LORA_RFSW_CTRL_PIN   GPIO_PIN_11

#define CONFIG_LORA_RFSW_VDD_GPIOX GPIOA
#define CONFIG_LORA_RFSW_VDD_PIN   GPIO_PIN_10

#define CONFIG_LWAN_KEYS_FLASH_ADDR        (0x0801F000)
#define CONFIG_LWAN_SETTINGS_FLASH_ADDR    (0x0801E000)

#define CONFIG_MANUFACTURER "ASR"
#define CONFIG_DEVICE_MODEL "6601"
#define CONFIG_VERSION "v1.2.0"

#ifdef __cplusplus
}
#endif

#endif /* __LORA_CONFIG_H */
