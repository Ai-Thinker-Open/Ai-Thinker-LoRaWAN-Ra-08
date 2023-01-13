/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */
#include <stdlib.h>
#include <string.h>
#include "log.h"
#include "LoRaMac.h"
#include "Region.h"
#include "tremo_flash.h"
#include "tremo_delay.h"
#include "tremo_uart.h"
#include "tremo_system.h"
#include "lwan_config.h"
#include "linkwan.h"
#include "linkwan_ica_at.h"

#define ARGC_LIMIT 16
#define ATCMD_SIZE (LORAWAN_APP_DATA_BUFF_SIZE * 2 + 18)
#define PORT_LEN 4

#define QUERY_CMD		0x01
#define EXECUTE_CMD		0x02
#define DESC_CMD        0x03
#define SET_CMD			0x04

uint8_t atcmd[ATCMD_SIZE];
uint16_t atcmd_index = 0;
volatile bool g_atcmd_processing = false;
uint8_t g_default_key[LORA_KEY_LENGTH] = {0x41, 0x53, 0x52, 0x36, 0x35, 0x30, 0x58, 0x2D, 
                                          0x32, 0x30, 0x31, 0x38, 0x31, 0x30, 0x33, 0x30};

typedef struct {
	char *cmd;
	int (*fn)(int opt, int argc, char *argv[]);	
}at_cmd_t;

//AT functions
static int at_cjoinmode_func(int opt, int argc, char *argv[]);
static int at_cdeveui_func(int opt, int argc, char *argv[]);
static int at_cappeui_func(int opt, int argc, char *argv[]);
static int at_cappkey_func(int opt, int argc, char *argv[]);
static int at_cdevaddr_func(int opt, int argc, char *argv[]);
static int at_cappskey_func(int opt, int argc, char *argv[]);
static int at_cnwkskey_func(int opt, int argc, char *argv[]);
static int at_caddmulticast_func(int opt, int argc, char *argv[]);
static int at_cdelmulticast_func(int opt, int argc, char *argv[]);
static int at_cnummulticast_func(int opt, int argc, char *argv[]);
static int at_cfreqbandmask_func(int opt, int argc, char *argv[]);
static int at_culdlmode_func(int opt, int argc, char *argv[]);
static int at_cworkmode_func(int opt, int argc, char *argv[]);
static int at_cclass_func(int opt, int argc, char *argv[]);
static int at_cbl_func(int opt, int argc, char *argv[]);
static int at_cstatus_func(int opt, int argc, char *argv[]);
static int at_cjoin_func(int opt, int argc, char *argv[]);
static int at_dtrx_func(int opt, int argc, char *argv[]);
static int at_drx_func(int opt, int argc, char *argv[]);
static int at_cconfirm_func(int opt, int argc, char *argv[]);
static int at_cappport_func(int opt, int argc, char *argv[]);
static int at_cdatarate_func(int opt, int argc, char *argv[]);
static int at_crssi_func(int opt, int argc, char *argv[]);
static int at_cnbtrials_func(int opt, int argc, char *argv[]);
static int at_crm_func(int opt, int argc, char *argv[]);
static int at_ctxp_func(int opt, int argc, char *argv[]);
static int at_clinkcheck_func(int opt, int argc, char *argv[]);
static int at_cadr_func(int opt, int argc, char *argv[]);
static int at_crxp_func(int opt, int argc, char *argv[]);
static int at_crx1delay_func(int opt, int argc, char *argv[]);
static int at_csave_func(int opt, int argc, char *argv[]);
static int at_crestore_func(int opt, int argc, char *argv[]);
static int at_cpslotinforeq_func(int opt, int argc, char *argv[]);
static int at_ckeysprotect_func(int opt, int argc, char *argv[]);
static int at_cgmi_func(int opt, int argc, char *argv[]);
static int at_cgmm_func(int opt, int argc, char *argv[]);
static int at_cgmr_func(int opt, int argc, char *argv[]);
static int at_cgsn_func(int opt, int argc, char *argv[]);
static int at_cgbr_func(int opt, int argc, char *argv[]);
static int at_iloglvl_func(int opt, int argc, char *argv[]);
static int at_ireboot_func(int opt, int argc, char *argv[]);

static at_cmd_t g_at_table[] = {
    {LORA_AT_CJOINMODE, at_cjoinmode_func},
    {LORA_AT_CDEVEUI,  at_cdeveui_func},
    {LORA_AT_CAPPEUI,  at_cappeui_func},
    {LORA_AT_CAPPKEY,  at_cappkey_func},
    {LORA_AT_CDEVADDR,  at_cdevaddr_func},
    {LORA_AT_CAPPSKEY,  at_cappskey_func},
    {LORA_AT_CNWKSKEY,  at_cnwkskey_func},
    {LORA_AT_CADDMUTICAST,  at_caddmulticast_func},
    {LORA_AT_CDELMUTICAST,  at_cdelmulticast_func},
    {LORA_AT_CNUMMUTICAST,  at_cnummulticast_func},
    {LORA_AT_CFREQBANDMASK,  at_cfreqbandmask_func},
    {LORA_AT_CULDLMODE,  at_culdlmode_func},
    {LORA_AT_CWORKMODE,  at_cworkmode_func},
    {LORA_AT_CCLASS,  at_cclass_func},
    {LORA_AT_CBL,  at_cbl_func},
    {LORA_AT_CSTATUS,  at_cstatus_func},
    {LORA_AT_CJOIN, at_cjoin_func},
    {LORA_AT_DTRX,  at_dtrx_func},
    {LORA_AT_DRX,  at_drx_func},
    {LORA_AT_CCONFIRM,  at_cconfirm_func},
    {LORA_AT_CAPPPORT,  at_cappport_func},
    {LORA_AT_CDATARATE,  at_cdatarate_func},
    {LORA_AT_CRSSI,  at_crssi_func},
    {LORA_AT_CNBTRIALS,  at_cnbtrials_func},
    {LORA_AT_CRM,  at_crm_func},
    {LORA_AT_CTXP,  at_ctxp_func},
    {LORA_AT_CLINKCHECK,  at_clinkcheck_func},
    {LORA_AT_CADR,  at_cadr_func},
    {LORA_AT_CRXP,  at_crxp_func},
    {LORA_AT_CRX1DELAY,  at_crx1delay_func},
    {LORA_AT_CSAVE,  at_csave_func},
    {LORA_AT_CRESTORE,  at_crestore_func},
    {LORA_AT_PINGSLOTINFOREQ,  at_cpslotinforeq_func},
    {LORA_AT_CKEYSPROTECT,  at_ckeysprotect_func},
    {LORA_AT_CGMI,  at_cgmi_func},
    {LORA_AT_CGMM,  at_cgmm_func},
    {LORA_AT_CGMR,  at_cgmr_func},
    {LORA_AT_CGSN,  at_cgsn_func},
    {LORA_AT_CGBR,  at_cgbr_func},
    {LORA_AT_ILOGLVL,  at_iloglvl_func},
    {LORA_AT_IREBOOT,  at_ireboot_func},
};

#define AT_TABLE_SIZE	(sizeof(g_at_table) / sizeof(at_cmd_t))

extern void uart_log_init(uint32_t baudrate);

static int hex2bin(const char *hex, uint8_t *bin, uint16_t bin_length)
{
    uint16_t hex_length = strlen(hex);
    const char *hex_end = hex + hex_length;
    uint8_t *cur = bin;
    uint8_t num_chars = hex_length & 1;
    uint8_t byte = 0;

    if ((hex_length + 1) / 2 > bin_length) {
        return -1;
    }

    while (hex < hex_end) {
        if ('A' <= *hex && *hex <= 'F') {
            byte |= 10 + (*hex - 'A');
        } else if ('a' <= *hex && *hex <= 'f') {
            byte |= 10 + (*hex - 'a');
        } else if ('0' <= *hex && *hex <= '9') {
            byte |= *hex - '0';
        } else {
            return -1;
        }
        hex++;
        num_chars++;

        if (num_chars >= 2) {
            num_chars = 0;
            *cur++ = byte;
            byte = 0;
        } else {
            byte <<= 4;
        }
    }
    return cur - bin;
}

// this can be in intrpt context
void linkwan_serial_input(uint8_t cmd)
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

int linkwan_serial_output(uint8_t *buffer, int len)
{
    AT_PRINTF("%s", buffer);
    linkwan_at_prompt_print();
    return 0;
}

void linkwan_at_prompt_print()
{
    LOG_PRINTF(LL_DEBUG, "\r\n%s%s:~# ", CONFIG_MANUFACTURER, CONFIG_DEVICE_MODEL);
}

static int at_cjoinmode_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t join_mode;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_dev_config_get(DEV_CONFIG_JOIN_MODE, &join_mode);  
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%d\r\nOK\r\n", LORA_AT_CJOINMODE, join_mode);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:\"mode\"\r\nOK\r\n", LORA_AT_CJOINMODE);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            join_mode = strtol((const char *)argv[0], NULL, 0);
            int res = lwan_dev_config_set(DEV_CONFIG_JOIN_MODE, (void *)&join_mode);
            if (res == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
            }
            break;
        }
        default: break;
    }
    
    return ret;
}

static int at_cdeveui_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t length;
    uint8_t buf[16];
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_dev_keys_get(DEV_KEYS_OTA_DEVEUI, buf);
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%02X%02X%02X%02X%02X%02X%02X%02X\r\nOK\r\n", \
                     LORA_AT_CDEVEUI, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s=\"DevEUI:length is 16\"\r\n", LORA_AT_CDEVEUI);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            length = hex2bin((const char *)argv[0], buf, LORA_EUI_LENGTH);
            if (length == LORA_EUI_LENGTH) {
                if(lwan_dev_keys_set(DEV_KEYS_OTA_DEVEUI, buf) == LWAN_SUCCESS) {
                    snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
                    ret = LWAN_SUCCESS;
                }
            }
            break;
        }
        default: break;
    }
    
    return ret;
}

static int at_cappeui_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t length;
    uint8_t buf[16];
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_dev_keys_get(DEV_KEYS_OTA_APPEUI, buf);
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%02X%02X%02X%02X%02X%02X%02X%02X\r\nOK\r\n", \
                     LORA_AT_CAPPEUI, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s=\"AppEUI:length is 16\"\r\n", LORA_AT_CAPPEUI);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            length = hex2bin((const char *)argv[0], buf, LORA_EUI_LENGTH);
            if (length == LORA_EUI_LENGTH && lwan_dev_keys_set(DEV_KEYS_OTA_APPEUI, buf) == LWAN_SUCCESS) {
                snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
                ret = LWAN_SUCCESS;
            }
            break;
        }
        default: break;
    }

    return ret;
}

static int at_cappkey_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t length;
    uint8_t buf[16];
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_dev_keys_get(DEV_KEYS_OTA_APPKEY, buf);   
            snprintf((char *)atcmd, ATCMD_SIZE,
                     "\r\n%s:%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\nOK\r\n", \
                     LORA_AT_CAPPKEY, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
                                      buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s=\"AppKey:length is 32\"\r\n", LORA_AT_CAPPKEY);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            length = hex2bin((const char *)argv[0], buf, LORA_KEY_LENGTH);
            if (length == LORA_KEY_LENGTH && lwan_dev_keys_set(DEV_KEYS_OTA_APPKEY, buf) == LWAN_SUCCESS) {
                snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
                ret = LWAN_SUCCESS;
            }
            break;
        }
        default: break;
    }

    return ret;
}

static int at_cdevaddr_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t length;
    uint8_t buf[16];
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            uint32_t devaddr;
            lwan_dev_keys_get(DEV_KEYS_ABP_DEVADDR, &devaddr);
            length = snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%08X\r\nOK\r\n", LORA_AT_CDEVADDR, (unsigned int)devaddr);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s=<DevAddr:length is 8, Device address of ABP mode>\r\n", LORA_AT_CDEVADDR);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            length = hex2bin((const char *)argv[0], buf, 4);
            if (length == 4) {
                uint32_t devaddr = buf[0] << 24 | buf[1] << 16 | buf[2] <<8 | buf[3];
                if(lwan_dev_keys_set(DEV_KEYS_ABP_DEVADDR, &devaddr) == LWAN_SUCCESS) {
                    snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
                    ret = LWAN_SUCCESS;
                }
            }
            break;
        }
        default: break;
    }

    return ret;
}

static int at_cappskey_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t length;
    uint8_t buf[16];
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_dev_keys_get(DEV_KEYS_ABP_APPSKEY, buf); 
            length = snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:", LORA_AT_CAPPSKEY);
            for (int i = 0; i < LORA_KEY_LENGTH; i++) {
                length += snprintf((char *)(atcmd + length), ATCMD_SIZE, "%02X", buf[i]);
            }
            snprintf((char *)(atcmd + length), ATCMD_SIZE, "\r\nOK\r\n");
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s=<AppSKey: length is 32>\r\n", LORA_AT_CAPPSKEY);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            length = hex2bin((const char *)argv[0], buf, LORA_KEY_LENGTH);
            if (length == LORA_KEY_LENGTH) {
                if(lwan_dev_keys_set(DEV_KEYS_ABP_APPSKEY, buf) == LWAN_SUCCESS) {
                    snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
                    ret = LWAN_SUCCESS;
                }
            }
            break;
        }
        default: break;
    }

    return ret;
}


static int at_cnwkskey_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t length;
    uint8_t buf[16];
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_dev_keys_get(DEV_KEYS_ABP_NWKSKEY, buf); 
            length = snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:", LORA_AT_CNWKSKEY);
            for (int i = 0; i < LORA_KEY_LENGTH; i++) {
                length += snprintf((char *)(atcmd + length), ATCMD_SIZE, "%02X", buf[i]);
            }
            snprintf((char *)(atcmd + length), ATCMD_SIZE, "\r\nOK\r\n");
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s=<NwkSKey:length is 32>\r\n", LORA_AT_CNWKSKEY);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            length = hex2bin((const char *)argv[0], buf, LORA_KEY_LENGTH);
            if (length == LORA_KEY_LENGTH) {
                if(lwan_dev_keys_set(DEV_KEYS_ABP_NWKSKEY, buf) == LWAN_SUCCESS) {
                    snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
                    ret = LWAN_SUCCESS;
                }
            }
            break;
        }
        default: break;
    }
 
    return ret;
}

static int at_caddmulticast_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
        
    switch(opt) {
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:\"DevAddr\",\"AppSKey\",\"NwkSKey\"\r\nOK\r\n", LORA_AT_CADDMUTICAST);
            break;
        }
        case SET_CMD: {
            if(argc < 3) break;
            
            MulticastParams_t curMulticastInfo;
            curMulticastInfo.Address = (uint32_t)strtoul(argv[0], NULL, 16);
            uint8_t appskey_len = hex2bin((const char *)argv[1], curMulticastInfo.AppSKey, 16);
            uint8_t nwkskey_len = hex2bin((const char *)argv[2], curMulticastInfo.NwkSKey, 16);
            
            if(argc > 3) {
                curMulticastInfo.Periodicity = strtoul(argv[3], NULL, 16);
                if(argc > 4 )
                    curMulticastInfo.Datarate = strtoul(argv[4], NULL, 16);
                curMulticastInfo.Frequency = 0;
            }
            
            if (appskey_len == 16 && nwkskey_len == 16) {
                ret = LWAN_SUCCESS;
                lwan_multicast_add(&curMulticastInfo);
                snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
            }
            
            break;
        }
        default: break;
    }

    return ret;
}

static int at_cdelmulticast_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
        
    switch(opt) {
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:\"DevAddr\"\r\nOK\r\n", LORA_AT_CDELMUTICAST);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            uint32_t devAddr = (uint32_t)strtol((const char *)argv[0], NULL, 16);
            if (lwan_multicast_del(devAddr) == true) {
                snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
                ret = LWAN_SUCCESS;
            }
            break;
        }
        default: break;
    }
 
    return ret;
}

static int at_cnummulticast_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            uint8_t multiNum = lwan_multicast_num_get();
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%u\r\nOK\r\n", LORA_AT_CNUMMUTICAST, multiNum);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:\"number\"\r\nOK\r\n", LORA_AT_CNUMMUTICAST);
            break;
        }
        default: break;
    }

    return ret;
}

static int at_cfreqbandmask_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t length;
    uint16_t freqband_mask;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_dev_config_get(DEV_CONFIG_FREQBAND_MASK, &freqband_mask);  
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%04X\r\nOK\r\n", LORA_AT_CFREQBANDMASK, freqband_mask);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s=\"mask\"\r\nOK\r\n", LORA_AT_CFREQBANDMASK);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            uint8_t mask[2];
            length = hex2bin((const char *)argv[0], (uint8_t *)mask, 2);
            if (length == 2) {
                freqband_mask = mask[1] | ((uint16_t)mask[0] << 8);
                if (lwan_dev_config_set(DEV_CONFIG_FREQBAND_MASK, (void *)&freqband_mask) == LWAN_SUCCESS) {
                    ret = LWAN_SUCCESS;
                    snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
                }
            }
            break;
        }
        default: break;
    }

    return ret;
}

static int at_culdlmode_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t mode;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_dev_config_get(DEV_CONFIG_ULDL_MODE, &mode);
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%d\r\nOK\r\n", LORA_AT_CULDLMODE, mode);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s=\"mode\"\r\nOK\r\n", LORA_AT_CULDLMODE);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            mode = strtol((const char *)argv[0], NULL, 0);
            if (lwan_dev_config_set(DEV_CONFIG_ULDL_MODE, (void *)&mode) == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
            }
            break;
        }
        default: break;
    }

    return ret;
}

static int at_cworkmode_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t mode;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_dev_config_get(DEV_CONFIG_WORK_MODE, &mode);
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%d\r\nOK\r\n", LORA_AT_CWORKMODE, mode);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s=\"mode\"\r\nOK\r\n", LORA_AT_CWORKMODE);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            mode = strtol((const char *)argv[0], NULL, 0);
            if (mode == 2) {
                ret = LWAN_SUCCESS;
                snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
            }
            
            break;
        }
        default: break;
    }

    return ret;
}

static int at_cclass_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t class;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_dev_config_get(DEV_CONFIG_CLASS, &class);  
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%d\r\nOK\r\n", LORA_AT_CCLASS, class);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE,
                     "\r\n%s:\"class\",\"branch\",\"para1\",\"para2\",\"para3\",\"para4\"\r\nOK\r\n",
                     LORA_AT_CCLASS);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            int8_t branch = -1;
            uint32_t param1 = 0;
            uint32_t param2 = 0;
            uint32_t param3 = 0;
            uint32_t param4 = 0;
            class = strtol((const char *)argv[0], NULL, 0);
            if(class == CLASS_B) {
                if(argc>1) {
                    branch = strtol((const char *)argv[1], NULL, 0);
                    param1 = strtol((const char *)argv[2], NULL, 0);
                    if(branch == 1 ){
                        param2 = strtol((const char *)argv[3], NULL, 0);
                        param3 = strtol((const char *)argv[4], NULL, 0);
                        param4 = strtol((const char *)argv[5], NULL, 0);
                    }
                }
            }

            if (lwan_dev_config_set(DEV_CONFIG_CLASS, (void *)&class) == LWAN_SUCCESS) {
                ClassBParam_t classb_param;
                lwan_dev_config_get(DEV_CONFIG_CLASSB_PARAM, (void *)&classb_param);
                if(branch==0){
                    classb_param.periodicity = param1;
                }else{
                    classb_param.beacon_freq = param1;
                    classb_param.beacon_dr = param2;
                    classb_param.pslot_freq = param3;
                    classb_param.pslot_dr = param4;
                }
                if (lwan_dev_config_set(DEV_CONFIG_CLASSB_PARAM, (void *)&classb_param) == LWAN_SUCCESS) {
                    ret = LWAN_SUCCESS;
                    snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
                }
            }
            
            break;
        }
        default: break;
    }

    return ret;
}

static int at_cbl_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            uint8_t batteryLevel = lwan_dev_battery_get();
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%d\r\nOK\r\n", LORA_AT_CBL, batteryLevel);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:\"value\"\r\nOK\r\n", LORA_AT_CBL);
            break;
        }
        default: break;
    }

    return ret;
}

static int at_cstatus_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    int status;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            status = lwan_dev_status_get();
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%02d\r\nOK\r\n", LORA_AT_CSTATUS, status);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:\"status\"\r\nOK\r\n", LORA_AT_CSTATUS);
            break;
        }
        default: break;
    }
 
    return ret;
}

static int at_dtrx_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
        
    switch(opt) {
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:[confirm],[nbtrials],[length],<payload>\r\nOK\r\n", LORA_AT_DTRX);
            break;
        }
        case SET_CMD: {
            if(argc < 2) break;
            
            uint8_t confirm, Nbtrials;
            uint16_t len;
            int bin_len = 0;
            uint8_t payload[LORAWAN_APP_DATA_BUFF_SIZE];
            confirm = strtol((const char *)argv[0], NULL, 0);
            Nbtrials = strtol((const char *)argv[1], NULL, 0);
            len = strtol((const char *)argv[2], NULL, 0);
            bin_len = hex2bin((const char *)argv[3], payload, len);
            
            if(bin_len>=0) {
                ret = LWAN_SUCCESS;
                if (lwan_data_send(confirm, Nbtrials, payload, bin_len) == LWAN_SUCCESS) {
                    snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK+SEND:%02X\r\n", bin_len);
                }else{
                    snprintf((char *)atcmd, ATCMD_SIZE, "\r\nERR+SEND:00\r\n");
                }
            }
            break;
        }
        default: break;
    }

    return ret;
}

static int at_drx_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
        
    switch(opt) {
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:<Length>,<Payload>\r\nOK\r\n", LORA_AT_DRX);
            break;
        }
        case QUERY_CMD: {
            uint8_t port;
            uint8_t *buf;
            uint8_t size;
                    
            if(lwan_data_recv(&port, &buf, &size) == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                int16_t len = snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%d", LORA_AT_DRX, size);
                if (size > 0) {
                    len += snprintf((char *)(atcmd + len), ATCMD_SIZE, ",");
                    for(int i=0; i<size; i++) {
                        len += snprintf((char *)(atcmd + len), ATCMD_SIZE, "%02X", buf[i]);
                    }
                }
                snprintf((char *)(atcmd + len), ATCMD_SIZE, "\r\n%s\r\n", "OK");
            }
            break;
        }
        default: break;
    }

    return ret;

}


static int at_cconfirm_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t cfm;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_mac_config_get(MAC_CONFIG_CONFIRM_MSG, &cfm);
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%d\r\nOK\r\n", LORA_AT_CCONFIRM, cfm);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:\"value\"\r\nOK\r\n", LORA_AT_CCONFIRM);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            cfm = strtol((const char *)argv[0], NULL, 0);
            if (lwan_mac_config_set(MAC_CONFIG_CONFIRM_MSG, (void *)&cfm) == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
            }
            
            break;
        }
        default: break;
    }

    return ret;
}


static int at_cappport_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t port;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_mac_config_get(MAC_CONFIG_APP_PORT, &port);
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%d\r\nOK\r\n", LORA_AT_CAPPPORT, port);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:\"value\"\r\nOK\r\n", LORA_AT_CAPPPORT);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            port = (uint8_t)strtol((const char *)argv[0], NULL, 0);

            if (lwan_mac_config_set(MAC_CONFIG_APP_PORT, (void *)&port) == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
            }
            break;
        }
        default: break;
    }

    return ret;
}


static int at_cdatarate_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t datarate;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_mac_config_get(MAC_CONFIG_DATARATE, &datarate);
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%d\r\nOK\r\n", LORA_AT_CDATARATE, datarate);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:\"value\"\r\nOK\r\n", LORA_AT_CDATARATE);
            break;
        }
        case SET_CMD: {
            uint8_t adr = 0;
            if(argc < 1) break;
            
            lwan_mac_config_get(MAC_CONFIG_ADR_ENABLE, &adr);
            if(adr) break;
            
            datarate = strtol((const char *)argv[0], NULL, 0);
            if (lwan_mac_config_set(MAC_CONFIG_DATARATE, (void *)&datarate) == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
            }
            break;
        }
        default: break;
    }

    return ret;
}


static int at_crssi_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
        
    switch(opt) {
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s\r\nOK\r\n", LORA_AT_CRSSI);
            break;
        }
        case EXECUTE_CMD: {
            if(!argv[0]) break;
            
            uint8_t argv_len = strlen(argv[0]);
            if(argv_len > 2){
                char freq_str[4];
                int16_t channel_rssi[8];
                strncpy(freq_str, (const char *)argv[0], argv_len-1);
                freq_str[argv_len-1] = '\0';
                uint8_t freq_band = strtol((const char *)freq_str, NULL, 0);
                lwan_dev_rssi_get(freq_band, channel_rssi);
                snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:\r\n0:%d\r\n1:%d\r\n2:%d\r\n3:%d\r\n4:%d\r\n5:%d\r\n6:%d\r\n7:%d\r\nOK\r\n", \
                         LORA_AT_CRSSI, channel_rssi[0], channel_rssi[1], channel_rssi[2], channel_rssi[3], channel_rssi[4], channel_rssi[5],
                         channel_rssi[6], channel_rssi[7]);
                ret = LWAN_SUCCESS;
            }
            
            break;
        }
        default: break;
    }

    return ret;
}

static int at_cnbtrials_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t m_type, value;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_mac_config_get(MAC_CONFIG_CONFIRM_MSG, &m_type);
            if(m_type)
                lwan_mac_config_get(MAC_CONFIG_CONF_NBTRIALS, &value);
            else
                lwan_mac_config_get(MAC_CONFIG_UNCONF_NBTRIALS, &value);
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%d,%d\r\nOK\r\n", LORA_AT_CNBTRIALS, m_type, value);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:\"MTypes\",\"value\"\r\nOK\r\n", LORA_AT_CNBTRIALS);
            break;
        }
        case SET_CMD: {
            if(argc < 2) break;
            
            m_type = strtol((const char *)argv[0], NULL, 0);
            value = strtol((const char *)argv[1], NULL, 0);
            int res = LWAN_SUCCESS;
            if(m_type)
                res = lwan_mac_config_set(MAC_CONFIG_CONF_NBTRIALS, (void *)&value);   
            else
                res = lwan_mac_config_set(MAC_CONFIG_UNCONF_NBTRIALS, (void *)&value);
            if (res == LWAN_SUCCESS) { 
                ret = LWAN_SUCCESS;
                snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
            }
            break;
        }
        default: break;
    }

    return ret;
}


static int at_crm_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t reportMode;
    uint32_t reportInterval;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_mac_config_get(MAC_CONFIG_REPORT_MODE, &reportMode);
            lwan_mac_config_get(MAC_CONFIG_REPORT_INTERVAL, &reportInterval);
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%d,%u\r\nOK\r\n", LORA_AT_CRM, reportMode, (unsigned int)reportInterval);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:\"reportMode\",\"reportInterval\"\r\nOK\r\n", LORA_AT_CRM);
            break;
        }
        case SET_CMD: {
            if(argc < 2) break;
            
            reportMode = strtol((const char *)argv[0], NULL, 0);
            reportInterval = strtol((const char *)argv[1], NULL, 0);
            lwan_mac_config_set(MAC_CONFIG_REPORT_MODE, (void *)&reportMode);
            if (lwan_mac_config_set(MAC_CONFIG_REPORT_INTERVAL, (void *)&reportInterval) == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
            }

            break;
        }
        default: break;
    }
    
    return ret;
}

static int at_ctxp_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t tx_power;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_mac_config_get(MAC_CONFIG_TX_POWER, &tx_power);
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%d\r\nOK\r\n", LORA_AT_CTXP, tx_power);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:\"value\"\r\nOK\r\n", LORA_AT_CTXP);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            tx_power = strtol((const char *)argv[0], NULL, 0);
            if (lwan_mac_config_set(MAC_CONFIG_TX_POWER, (void *)&tx_power) == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
            }
            break;
        }
        default: break;
    }

    return ret;
}

static int at_clinkcheck_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t checkValue;
        
    switch(opt) {
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:\"value\"\r\nOK\r\n", LORA_AT_CLINKCHECK);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            checkValue = strtol((const char *)argv[0], NULL, 0);
            if (lwan_mac_config_set(MAC_CONFIG_CHECK_MODE, (void *)&checkValue) == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                if(checkValue==1)
                    lwan_mac_req_send(MAC_REQ_LINKCHECK, 0);
                snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
            }
            break;
        }
        default: break;
    }

    return ret;
}

static int at_cadr_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t adr;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_mac_config_get(MAC_CONFIG_ADR_ENABLE, &adr);
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%d\r\nOK\r\n", LORA_AT_CADR, adr);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:\"value\"\r\nOK\r\n", LORA_AT_CADR);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            adr = strtol((const char *)argv[0], NULL, 0);
            if (lwan_mac_config_set(MAC_CONFIG_ADR_ENABLE, (void *)&adr) == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
            }
            break;
        }
        default: break;
    }

    return ret;
}

static int at_crxp_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    RxParams_t rx_params;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_mac_config_get(MAC_CONFIG_RX_PARAM, &rx_params);  
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%u,%u,%u\r\nOK\r\n", LORA_AT_CRXP, rx_params.rx1_dr_offset, rx_params.rx2_dr, (unsigned int)rx_params.rx2_freq);            
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:\"RX1DRoffset\",\"RX2DataRate\",\"RX2Frequency\"\r\nOK\r\n", LORA_AT_CRXP);
            break;
        }
        case SET_CMD: {
            if(argc < 3) break;
            
            uint8_t rx1_dr_offset;
            uint8_t rx2_dr;
            uint32_t rx2_freq;                            
            rx1_dr_offset = strtol((const char *)argv[0], NULL, 0);
            rx2_dr = strtol((const char *)argv[1], NULL, 0);
            rx2_freq = strtol((const char *)argv[2], NULL, 0);
            rx_params.rx1_dr_offset = rx1_dr_offset;
            rx_params.rx2_dr = rx2_dr;
            rx_params.rx2_freq = rx2_freq;
            if (lwan_mac_config_set(MAC_CONFIG_RX_PARAM, (void *)&rx_params) == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
            }     
            break;
        }
        default: break;
    }
 
    return ret;
}

static int at_crx1delay_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint16_t rx1delay;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_mac_config_get(MAC_CONFIG_RX1_DELAY, &rx1delay);
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%u\r\nOK\r\n", LORA_AT_CRX1DELAY, (unsigned int)rx1delay);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:\"value\"\r\nOK\r\n", LORA_AT_CRX1DELAY);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            rx1delay = strtol((const char *)argv[0], NULL, 0);
            if (lwan_mac_config_set(MAC_CONFIG_RX1_DELAY, (void *)&rx1delay) == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
            }
            break;
        }
        default: break;
    }
 
    return ret;
}

static int at_csave_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
        
    switch(opt) {
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s\r\nOK\r\n", LORA_AT_CSAVE);
            break;
        }
        case EXECUTE_CMD: {
            if (lwan_mac_config_save() == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
            }
            break;
        }
        default: break;
    }

    return ret;
}

static int at_crestore_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
        
    switch(opt) {
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s\r\nOK\r\n", LORA_AT_CRESTORE);
            break;
        }
        case EXECUTE_CMD: {
            LWanMacConfig_t default_mac_config = LWAN_MAC_CONFIG_DEFAULT;
            if (lwan_mac_config_reset(&default_mac_config) == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
            }
            break;
        }
        default: break;
    }
 
    return ret;
}

static int at_cpslotinforeq_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            ClassBParam_t classb_param;
            lwan_dev_config_get(DEV_CONFIG_CLASSB_PARAM, &classb_param);
            snprintf((char *)atcmd,ATCMD_SIZE,"\r\n%s:%u\r\nOK\r\n", LORA_AT_PINGSLOTINFOREQ, classb_param.periodicity);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:<periodicity>\r\nOK\r\n", LORA_AT_PINGSLOTINFOREQ);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            uint8_t periodicityVal = (uint8_t)strtol((const char *)argv[0], NULL, 0);
            if (lwan_mac_req_send(MAC_REQ_PSLOT_INFO, &periodicityVal) == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
            }
            break;
        }
        default: break;
    }
 
    return ret;
}

static int at_ckeysprotect_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t length;
    uint8_t buf[16];
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_dev_keys_get(DEV_KEYS_PKEY, buf);
            bool protected = lwan_is_key_valid(buf, LORA_KEY_LENGTH);
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%d\r\nOK\r\n", LORA_AT_CKEYSPROTECT, protected);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s=<ProtectKey:length is 32>\r\n", LORA_AT_CKEYSPROTECT);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            int res = LWAN_SUCCESS;
            length = hex2bin((const char *)argv[0], buf, LORA_KEY_LENGTH);
            if (length == LORA_KEY_LENGTH) {
                res = lwan_dev_keys_set(DEV_KEYS_PKEY, buf);
            } else {
                res = lwan_dev_keys_set(DEV_KEYS_PKEY, g_default_key);
            }
            
            if (res == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
            }
            break;
        }
        default: break;
    }
    
    return ret;
}

static int at_cjoin_func(int opt, int argc, char *argv[])
{
    int ret = -1;
    uint8_t bJoin, bAutoJoin;
    uint16_t joinInterval, joinRetryCnt;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = 0;
            JoinSettings_t join_settings;
            lwan_dev_config_get(DEV_CONFIG_JOIN_SETTINGS, &join_settings);
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%d,%d,%d,%d\r\nOK\r\n", LORA_AT_CJOIN, join_settings.auto_join, 
                     join_settings.auto_join, join_settings.join_interval, join_settings.join_trials);
            break;
        }
        case DESC_CMD: {
            ret = 0;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:<ParaTag1>,[ParaTag2],[ParaTag3],[ParaTag4]\r\nOK\r\n", LORA_AT_CJOIN);
            break;
        }
        case SET_CMD: {
            if(argc < 4) break;
            
            bJoin = strtol((const char *)argv[0], NULL, 0);
            bAutoJoin = strtol((const char *)argv[1], NULL, 0);
            joinInterval = strtol((const char *)argv[2], NULL, 0);
            joinRetryCnt = strtol((const char *)argv[3], NULL, 0);
            
            int res = lwan_join(bJoin, bAutoJoin, joinInterval, joinRetryCnt);
            if (res == LWAN_SUCCESS) {
                ret = 0;
                snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
            }    
            break;
        }
        default: break;
    }
    
    return ret;
}

static int at_cgmi_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    
    
    
    if(opt == QUERY_CMD) {
        ret = LWAN_SUCCESS;

        snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s=%s\r\nOK\r\n", LORA_AT_CGMI, CONFIG_MANUFACTURER);
    }
    
    return ret;
}

static int at_cgmm_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    
    if(opt == QUERY_CMD) {
        ret = LWAN_SUCCESS;
        snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s=%s\r\nOK\r\n", LORA_AT_CGMM, CONFIG_DEVICE_MODEL);
    }
    return ret;
}

static int at_cgmr_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    
    if(opt == QUERY_CMD) {
        ret = LWAN_SUCCESS;
        snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s=%s\r\nOK\r\n", LORA_AT_CGMR, CONFIG_VERSION);
    }

    return ret;
}

static int at_cgsn_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    
    if(opt == QUERY_CMD) {
        uint32_t sn[2];
        uint8_t *buf = (uint8_t *)sn;
        ret = LWAN_SUCCESS;
        
        system_get_chip_id(sn);
        snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s=%02X%02X%02X%02X%02X%02X%02X%02X\r\nOK\r\n", \
                     LORA_AT_CGSN, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
    }
    return ret;
}

static int at_cgbr_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint32_t baud;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_sys_config_get(SYS_CONFIG_BAUDRATE, &baud);
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%u\r\nOK\r\n", LORA_AT_CGBR, (unsigned int)baud);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:\"value\"\r\nOK\r\n", LORA_AT_CGBR);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            baud = strtol((const char *)argv[0], NULL, 0);   
            if(baud<110 || baud>9600) break;
            
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
            linkwan_serial_output(atcmd, strlen((const char *)atcmd));
            
            uart_log_init(baud); 
            lwan_sys_config_set(SYS_CONFIG_BAUDRATE, &baud);
             
            return 1;
        }
        default: break;
    }
    
    return ret;
}

static int at_iloglvl_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            uint16_t ll = 0;
            lwan_sys_config_get(SYS_CONFIG_LOGLVL, &ll);
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:%d\r\nOK\r\n", LORA_AT_ILOGLVL, ll);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:\"level\"\r\nOK\r\n", LORA_AT_ILOGLVL);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
         
            ret = LWAN_SUCCESS;
            uint16_t ll = strtol((const char *)argv[0], NULL, 0);
            log_set_level((1<<ll)-1);
            
            lwan_sys_config_set(SYS_CONFIG_LOGLVL, &ll);
            
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
            break;
        }
        default: break;
    }
    
    return ret;
}

static int at_ireboot_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
        
    switch(opt) {
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s:\"Mode\"\r\nOK\r\n", LORA_AT_IREBOOT);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            int8_t mode = strtol((const char *)argv[0], NULL, 0);
            if (mode == 0 || mode == 1) {
                ret = LWAN_SUCCESS;
                snprintf((char *)atcmd, ATCMD_SIZE, "\r\nOK\r\n");
                AT_PRINTF("%s", atcmd);
                delay_ms(1);
                atcmd_index = 0;
                memset(atcmd, 0xff, ATCMD_SIZE);
                lwan_sys_reboot(mode);
            }
            break;
        }
        default: break;
    }

    return ret;
}

void linkwan_at_process(void)
{
    char *ptr = NULL;
	int argc = 0;
	int index = 0;
	char *argv[ARGC_LIMIT];
    int ret = LWAN_ERROR;
    uint8_t *rxcmd = atcmd + 2;
    int16_t rxcmd_index = atcmd_index - 2;

    if (atcmd_index <=2 && atcmd[atcmd_index] == '\0') {
        linkwan_at_prompt_print();
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
		ret = LWAN_ERROR;
	}

at_end:
	if (LWAN_ERROR == ret)
        snprintf((char *)atcmd, ATCMD_SIZE, "\r\n%s%x\r\n", AT_ERROR, 1);
    if( ret<=0 )
        linkwan_serial_output(atcmd, strlen((const char *)atcmd));  
        
    atcmd_index = 0;
    memset(atcmd, 0xff, ATCMD_SIZE);
    g_atcmd_processing = false;        
    return;
}

void linkwan_at_init(void)
{
    atcmd_index = 0;
    memset(atcmd, 0xff, ATCMD_SIZE);
}