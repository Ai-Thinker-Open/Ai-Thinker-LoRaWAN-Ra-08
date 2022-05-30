/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#ifndef __LWAN_H__
#define __LWAN_H__

#include <stdbool.h>
#include <stdio.h>
#include "lora_config.h"

#define LORA_EUI_LENGTH 8
#define LORA_KEY_LENGTH 16
    
#define LWAN_SUCCESS  0
#define LWAN_ERROR   -1    

#ifdef CONFIG_LINKWAN
#define LWAN_PROTL   1 
#else
#define LWAN_PROTL   0 
#endif

#define LWAN_KEYS_FLASH_ADDR        CONFIG_LWAN_KEYS_FLASH_ADDR
#define LWAN_SETTINGS_FLASH_ADDR    CONFIG_LWAN_SETTINGS_FLASH_ADDR
#define LWAN_SETTINGS_DEV            0
#define LWAN_SETTINGS_MAC            1
#define LWAN_SETTINGS_SYS            2

#define LWAN_DEV_KEYS_DEFAULT   {LORA_KEYS_MAGIC_NUM, {0}, \
                                 {LORAWAN_DEVICE_EUI, LORAWAN_APPLICATION_EUI, LORAWAN_APPLICATION_KEY}, \
                                 {LORAWAN_DEVICE_ADDRESS, LORAWAN_NWKSKEY, LORAWAN_APPSKEY}, 0}

#define LWAN_DEV_CONFIG_DEFAULT {{JOIN_MODE_OTAA, ULDL_MODE_INTRA, WORK_MODE_NORMAL, CLASS_A}, \
                                 {3, DR_2, DR_3, 0, 0}, {0, 8, 8, JOIN_METHOD_DEF, 1, DR_3}, \
                                 0x0001, 0}

#define LWAN_MAC_CONFIG_DEFAULT {{1, 0, 0, 1}, {7, 7}, 10, DR_3, 0, 0, {0, 0, 0}, 0, 0}

#define LWAN_SYS_CONFIG_DEFAULT {9600, 3,0}
    
#define LWAN_PRODCT_CONFIG_DEFAULT {LWAN_PROTL}

typedef enum JoinMode_s {
    JOIN_MODE_OTAA,
    JOIN_MODE_ABP
} JoinMode_t;

typedef enum eWorkMode {
    WORK_MODE_REPEATER = 1,  // Alibaba Node Repeater
    WORK_MODE_NORMAL = 2,
    WORK_MODE_AUTOSWITCH = 3,  // switch between normal and repeater
} WorkMode_t;

typedef enum eULDLMode {
    ULDL_MODE_INTRA = 1,  // uplink and downlink use same frequencies
    ULDL_MODE_INTER = 2,  // uplink and downlink use different frequencies
} ULDLMode_t;

typedef struct eClassBParam {
    int8_t periodicity;
    int8_t beacon_dr;
    int8_t pslot_dr;
    uint32_t beacon_freq;
    uint32_t pslot_freq;
} __attribute__((packed)) ClassBParam_t;

typedef enum eJoinMethod {
    JOIN_METHOD_STORED = 0,
    JOIN_METHOD_DEF = 1,
    JOIN_METHOD_SCAN = 2,
    JOIN_METHOD_NUM
} JoinMethod_t;

typedef struct sJoinSetings {
    bool auto_join;
    uint8_t join_interval;
    uint8_t join_trials;
    JoinMethod_t join_method;
    int8_t stored_freqband;
    uint8_t stored_datarate;
} JoinSettings_t;

typedef struct sLoraDevConfig {
    struct sDevModes {
        JoinMode_t join_mode :2;
        ULDLMode_t uldl_mode :2;
        WorkMode_t work_mode :2;
        uint8_t class_mode :2;
    } modes;
    ClassBParam_t classb_param;
    JoinSettings_t join_settings;
    uint16_t freqband_mask;
    uint16_t crc;
} __attribute__((packed)) LWanDevConfig_t;

typedef struct sRxParams {
    uint8_t rx1_dr_offset;
    uint8_t rx2_dr;
    uint32_t rx2_freq;
} RxParams_t;

typedef struct sLoraMacConfig {
    struct sMacModes {
        uint8_t confirmed_msg :1;
        uint8_t report_mode :1;
        uint8_t linkcheck_mode :2;
        uint8_t adr_enabled :1;
    } modes;
    struct sNbTrials {
        uint8_t conf: 4;
        uint8_t unconf: 4;
    } nbtrials;
    uint8_t port;
    uint8_t datarate;
    uint8_t tx_power;
    uint8_t rx1_delay;
    RxParams_t rx_params;
    uint32_t report_interval;
    uint16_t crc;
} __attribute__((packed)) LWanMacConfig_t;

typedef struct sLoraSysConfig {
    uint32_t baudrate;
    uint16_t loglevel;
    uint16_t crc;
} __attribute__((packed)) LWanSysConfig_t;

typedef struct sLoraProdctConfig {
    uint8_t protl;
} __attribute__((packed)) LWanProdctConfig_t;

typedef struct sOTAKeys {
    uint8_t deveui[LORA_EUI_LENGTH];
    uint8_t appeui[LORA_EUI_LENGTH];
    uint8_t appkey[LORA_KEY_LENGTH];
} OTAKeys_t;

typedef struct sABPKeys {
    uint32_t devaddr;
    uint8_t nwkskey[LORA_KEY_LENGTH];
    uint8_t appskey[LORA_KEY_LENGTH];
} ABPKeys_t;

typedef struct sLoraDevKeys {
    uint32_t magic;
    
    uint8_t pkey[LORA_KEY_LENGTH];
    OTAKeys_t ota;
    ABPKeys_t abp;
    
    uint16_t checksum;
} __attribute__((packed))LWanDevKeys_t;

typedef enum eDevKeysType {
    DEV_KEYS_OTA_DEVEUI = 0,
    DEV_KEYS_OTA_APPEUI,
    DEV_KEYS_OTA_APPKEY,
    DEV_KEYS_ABP_DEVADDR,
    DEV_KEYS_ABP_NWKSKEY,
    DEV_KEYS_ABP_APPSKEY,
    DEV_KEYS_PKEY,
    DEV_KEYS_MAX
} DevKeysType_t;

typedef enum eDevConfigType {
    DEV_CONFIG_JOIN_MODE = 0,
    DEV_CONFIG_FREQBAND_MASK,
    DEV_CONFIG_ULDL_MODE,
    DEV_CONFIG_WORK_MODE,
    DEV_CONFIG_CLASS,
    DEV_CONFIG_CLASSB_PARAM,
    DEV_CONFIG_JOIN_SETTINGS,
    DEV_CONFIG_MAX
} DevConfigType_t;

typedef enum eMacConfigType {
    MAC_CONFIG_CONFIRM_MSG = 0,
    MAC_CONFIG_APP_PORT,
    MAC_CONFIG_DATARATE,
    MAC_CONFIG_CONF_NBTRIALS,
    MAC_CONFIG_UNCONF_NBTRIALS,
    MAC_CONFIG_REPORT_MODE,
    MAC_CONFIG_REPORT_INTERVAL,
    MAC_CONFIG_TX_POWER,
    MAC_CONFIG_CHECK_MODE,
    MAC_CONFIG_ADR_ENABLE,
    MAC_CONFIG_RX_PARAM,
    MAC_CONFIG_RX1_DELAY,
    MAC_CONFIG_MAX
} MacConfigType_t;

typedef enum eSysConfigType {
    SYS_CONFIG_BAUDRATE = 0,
    SYS_CONFIG_LOGLVL,
    SYS_CONFIG_MAX
} SysConfigType_t;

bool lwan_is_key_valid(uint8_t *key,  uint8_t size);
LWanDevKeys_t *lwan_dev_keys_init(LWanDevKeys_t *default_keys);
int lwan_dev_keys_set(int type, void *data);
int lwan_dev_keys_get(int type, void *data);

LWanDevConfig_t *lwan_dev_config_init(LWanDevConfig_t *default_config);
LWanProdctConfig_t *lwan_prodct_config_init(LWanProdctConfig_t *default_config);
int lwan_dev_config_get(int type, void *config);
int lwan_dev_config_set(int type, void *config);

LWanMacConfig_t *lwan_mac_config_init(LWanMacConfig_t *default_config);
int lwan_mac_config_get(int type, void *config);
int lwan_mac_config_set(int type, void *config);
int lwan_mac_config_save();
int lwan_mac_config_reset(LWanMacConfig_t *default_config);

void lwan_mac_params_update();
void lwan_dev_params_update();

LWanSysConfig_t *lwan_sys_config_init(LWanSysConfig_t *default_config);
int lwan_sys_config_get(int type, void *config);
int lwan_sys_config_set(int type, void *config);

#endif /* __LWAN_H__ */
