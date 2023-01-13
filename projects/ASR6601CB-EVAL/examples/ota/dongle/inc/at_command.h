/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#ifndef LINKWAN_AT_H
#define LINKWAN_AT_H

#include <stdint.h>

#define AT_CMD "AT"

#define AT_ERROR "+CME ERROR:"

#define ATCMD_SIZE (255 * 2 + 18)

// mandatory
#define LORA_AT_FREQ "+FREQ"
#define LORA_AT_CFG "+CFG"
#define LORA_AT_TX "+TX"
#define LORA_AT_RX "+RX"


void at_init(void);
void at_process(void);
int serial_output(uint8_t *buffer, int len);
void serial_input(uint8_t cmd);

#endif
