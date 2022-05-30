/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "at_command.h"
#include "radio.h"

#define ARGC_LIMIT 16

#define PORT_LEN 4

#define QUERY_CMD        0x01
#define EXECUTE_CMD        0x02
#define DESC_CMD        0x03
#define SET_CMD            0x04

typedef struct {
    char *cmd;
    int (*fn)(int opt, int argc, char *argv[]);    
}at_cmd_t;

volatile bool g_atcmd_processing = false;
static uint8_t atcmd[ATCMD_SIZE];
static uint16_t atcmd_index;

//AT functions
extern int at_freq(int opt, int argc, char *argv[]);
extern int at_cfg(int opt, int argc, char *argv[]);
extern int at_tx(int opt, int argc, char *argv[]);
extern int at_rx(int opt, int argc, char *argv[]);

static const at_cmd_t g_at_table[] = {
    {LORA_AT_FREQ, at_freq},
    {LORA_AT_CFG, at_cfg},
    {LORA_AT_TX, at_tx},
    {LORA_AT_RX, at_rx},
};

#define AT_TABLE_SIZE    (sizeof(g_at_table) / sizeof(at_cmd_t))

// this can be in intrpt context
void serial_input(uint8_t cmd)
{
    if(g_atcmd_processing) 
        return;
    
    if ((cmd >= '0' && cmd <= '9') || (cmd >= 'a' && cmd <= 'z') ||
        (cmd >= 'A' && cmd <= 'Z') || cmd == '?' || cmd == '+' ||
        cmd == ':' || cmd == '=' || cmd == ' ' || cmd == ',') {
        if (atcmd_index >= ATCMD_SIZE) {
            memset(atcmd, 0xff, ATCMD_SIZE);
            atcmd_index = 0;
            return;
        }
        atcmd[atcmd_index++] = cmd;
    } else if (cmd == '\r' || cmd == '\n') {
        if (atcmd_index >= ATCMD_SIZE) {
            memset(atcmd, 0xff, ATCMD_SIZE);
            atcmd_index = 0;
            return;
        }
        atcmd[atcmd_index] = '\0';
    }
}

int serial_output(uint8_t *buffer, int len)
{
    printf("%s", buffer);
    return 0;
}

void at_process(void)
{
    char *ptr = NULL;
	int argc = 0;
	int index = 0;
	char *argv[ARGC_LIMIT];
    int ret = -1;
    uint8_t *rxcmd = atcmd + 2;
    int16_t rxcmd_index = atcmd_index - 2;

    if (atcmd_index <=2 && atcmd[atcmd_index] == '\0') {
        atcmd_index = 0;
        memset(atcmd, 0xff, ATCMD_SIZE);
        return;
    }
    
    if (rxcmd_index <= 0 || rxcmd[rxcmd_index] != '\0') {
        return;
    }

    g_atcmd_processing = true;
    
    if(atcmd[0] != 'A' || atcmd[1] != 'T')
        goto at_end;
    for (index = 0; index < AT_TABLE_SIZE; index++) {
        int cmd_len = strlen(g_at_table[index].cmd);
    	if (!strncmp((const char *)rxcmd, g_at_table[index].cmd, cmd_len)) {
    		ptr = (char *)rxcmd + cmd_len;
    		break;
    	}
    }
	if (index >= AT_TABLE_SIZE || !g_at_table[index].fn)
        goto at_end;

    if ((ptr[0] == '?') && (ptr[1] == '\0')) {
		ret = g_at_table[index].fn(QUERY_CMD, argc, argv);
	} else if (ptr[0] == '\0') {
		ret = g_at_table[index].fn(EXECUTE_CMD, argc, argv);
	}  else if (ptr[0] == ' ') {
        argv[argc++] = ptr;
		ret = g_at_table[index].fn(EXECUTE_CMD, argc, argv);
	} else if ((ptr[0] == '=') && (ptr[1] == '?') && (ptr[2] == '\0')) {
        ret = g_at_table[index].fn(DESC_CMD, argc, argv);
	} else if (ptr[0] == '=') {
		ptr += 1;
        
        char *str = strtok((char *)ptr, ",");
        while(str) {
            argv[argc++] = str;
            str = strtok((char *)NULL, ",");
        }
		ret = g_at_table[index].fn(SET_CMD, argc, argv);
	} else {
		ret = -1;
	}

at_end:
	if (-1 == ret)
        snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s%x\r\n", AT_ERROR, 1);
          
    atcmd_index = 0;
    memset(atcmd, 0xff, ATCMD_SIZE);
    g_atcmd_processing = false;        
    return;
}

void at_init(void)
{
    atcmd_index = 0;
    memset(atcmd, 0xff, ATCMD_SIZE);
}
