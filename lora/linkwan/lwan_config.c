#include <string.h>
#include "tremo_flash.h"
#include "radio.h"
#include "Commissioning.h"
#include "LoRaMac.h"
#include "LoRaMacClassB.h"
#include "LoRaMacCrypto.h"
#include "linkwan.h"
#include "lwan_config.h" 

#define MAX_BEACON_RETRY_TIMES 2
#define LORA_KEYS_MAGIC_NUM 0xABABBABA 

static LWanDevConfig_t g_lwan_dev_config;
static LWanMacConfig_t g_lwan_mac_config;
static LWanSysConfig_t g_lwan_sys_config;
static LWanDevKeys_t g_lwan_dev_keys;
static LWanProdctConfig_t g_lwan_prodct_config;

static uint16_t crc16(uint8_t *buffer, uint8_t length )
{
    const uint16_t polynom = 0x1021;
    uint16_t crc = 0x0000;

    for (uint8_t i = 0; i < length; ++i) {
        crc ^= ( uint16_t ) buffer[i] << 8;
        for (uint8_t j = 0; j < 8; ++j) {
            crc = (crc & 0x8000) ? (crc << 1) ^ polynom : (crc << 1);
        }
    }

    return crc;
}

#ifdef CONFIG_LINKWAN 
static uint8_t get_next_freqband(void)
{
    uint8_t freqband[16];
    uint8_t freqnum = 0;
    uint16_t mask = g_lwan_dev_config.freqband_mask;

    freqband[freqnum++] = 1; //1A2
    for (uint8_t i = 0; i < 16; i++) {
        if ((mask & (1 << i)) && i != 1) {
            freqband[freqnum++] = i;
        }
    }
    
    return freqband[randr(0,freqnum-1)];
}
#endif

int read_settings(int type, void *setting, int len)
{
    uint32_t offset = 0;
    switch(type){
        case LWAN_SETTINGS_MAC:{
            offset += sizeof(LWanDevConfig_t);
            break;
        }
        case LWAN_SETTINGS_SYS:{
            offset += sizeof(LWanDevConfig_t) + sizeof(LWanMacConfig_t);
            break;
        }
        case LWAN_SETTINGS_DEV:
        default:{
            break;
        }
    }
    
    memcpy(setting, (void *)(LWAN_SETTINGS_FLASH_ADDR+offset), len);
    
    return LWAN_SUCCESS;
}

int write_settings(int type, void *setting, int len)
{
    uint32_t offset = 0;
    uint32_t total_len = sizeof(LWanDevConfig_t) + sizeof(LWanMacConfig_t) + sizeof(LWanSysConfig_t);
    uint8_t buf[total_len];
    int status = 0;

    switch(type){
        case LWAN_SETTINGS_MAC:{
            offset += sizeof(LWanDevConfig_t);
            break;
        }
        case LWAN_SETTINGS_SYS:{
            offset += sizeof(LWanDevConfig_t) + sizeof(LWanMacConfig_t);
            break;
        }
        case LWAN_SETTINGS_DEV:
        default:{
            break;
        }
    }
    
    memcpy(buf, (void *)LWAN_SETTINGS_FLASH_ADDR, total_len);
    memcpy(buf+offset, setting, len);
    
    flash_erase_page(LWAN_SETTINGS_FLASH_ADDR);
    status = flash_program_bytes(LWAN_SETTINGS_FLASH_ADDR, buf, total_len);
    if(status != 0){
        LOG_PRINTF(LL_ERR, "Error writing settings\r\n");
        return LWAN_ERROR;
    }
    
    return LWAN_SUCCESS;
}

int write_lwan_dev_config(LWanDevConfig_t *dev_config)
{
    int ret = LWAN_SUCCESS;
    dev_config->crc = crc16((uint8_t *)dev_config, sizeof(LWanDevConfig_t) - 2);
    ret = write_settings(LWAN_SETTINGS_DEV, dev_config, sizeof(LWanDevConfig_t));
    return ret==0?LWAN_SUCCESS:LWAN_ERROR;
}

int read_lwan_dev_config(LWanDevConfig_t *dev_config)
{
    int len;
    int ret = LWAN_SUCCESS;
    uint16_t crc;

    memset(dev_config, 0, sizeof(LWanDevConfig_t));
    len = sizeof(LWanDevConfig_t);
    ret = read_settings(LWAN_SETTINGS_DEV, dev_config, len);
    if (ret == LWAN_SUCCESS) {
        crc = crc16((uint8_t *)dev_config, len - 2);
        if (crc != dev_config->crc)
            ret = LWAN_ERROR;
    }
    
    return ret;
}

int write_lwan_mac_config(LWanMacConfig_t *mac_config)
{
    int ret = LWAN_SUCCESS;
    mac_config->crc = crc16((uint8_t *)mac_config, sizeof(LWanMacConfig_t) - 2);
    ret = write_settings(LWAN_SETTINGS_MAC, mac_config, sizeof(LWanMacConfig_t));

    return ret==0?LWAN_SUCCESS:LWAN_ERROR;
}

int read_lwan_mac_config(LWanMacConfig_t *mac_config)
 {
    int len;
    int ret = LWAN_SUCCESS;
    uint16_t crc;

    memset(mac_config, 0, sizeof(LWanMacConfig_t));
    len = sizeof(LWanMacConfig_t);
    ret = read_settings(LWAN_SETTINGS_MAC, mac_config, len);
    if (ret == LWAN_SUCCESS) {
        crc = crc16((uint8_t *)mac_config, len - 2);
        if (crc != mac_config->crc)
            ret = LWAN_ERROR;
    }
    
    return ret;
}

int read_lwan_dev_keys(LWanDevKeys_t *keys)
{
    uint16_t crc;
    
    memcpy(keys, (LWanDevKeys_t *)LWAN_KEYS_FLASH_ADDR, sizeof(LWanDevKeys_t));
    
    if(keys->magic != LORA_KEYS_MAGIC_NUM) {
        return LWAN_ERROR;
    }
    
    crc = crc16((uint8_t *)keys, sizeof(LWanDevKeys_t) - 2);
    if(crc != keys->checksum) {
        LOG_PRINTF(LL_ERR, "crc is wrong\r\n");
        return LWAN_ERROR;
    }

    return LWAN_SUCCESS;
}
int write_lwan_dev_keys(LWanDevKeys_t *keys)
{
    int status;
    
    keys->magic = LORA_KEYS_MAGIC_NUM;
    keys->checksum = crc16((uint8_t *)keys, sizeof(LWanDevKeys_t) - 2);
    
    flash_erase_page(LWAN_KEYS_FLASH_ADDR);
    status = flash_program_bytes(LWAN_KEYS_FLASH_ADDR, (uint8_t *)keys, sizeof(LWanDevKeys_t));
    if(status != 0){
        LOG_PRINTF(LL_ERR, "Error writing lora keys\r\n");
        return LWAN_ERROR;
    }
    
    return LWAN_SUCCESS;
}

int encrypt_lwan_dev_keys(LWanDevKeys_t *loraKeys, uint8_t *key)
{
    uint32_t enc_devaddr = 0x11111111;
    uint8_t enc_dir = 1;
    uint32_t enc_fcnt = 1;
    if(!loraKeys || !key) 
        return -1;
    uint8_t encBuffer[LORA_KEY_LENGTH];
    
    memcpy(loraKeys->pkey, key, LORA_KEY_LENGTH);
    
    //ota
    LoRaMacPayloadEncrypt( loraKeys->ota.deveui, sizeof(loraKeys->ota.deveui), key, enc_devaddr, enc_dir, enc_fcnt, encBuffer );
    memcpy(loraKeys->ota.deveui, encBuffer, sizeof(loraKeys->ota.deveui));
    LoRaMacPayloadEncrypt( loraKeys->ota.appeui, sizeof(loraKeys->ota.appeui), key, enc_devaddr, enc_dir, enc_fcnt, encBuffer );
    memcpy(loraKeys->ota.appeui, encBuffer, sizeof(loraKeys->ota.appeui));
    LoRaMacPayloadEncrypt( loraKeys->ota.appkey, sizeof(loraKeys->ota.appkey), key, enc_devaddr, enc_dir, enc_fcnt, encBuffer );
    memcpy(loraKeys->ota.appkey, encBuffer, sizeof(loraKeys->ota.appkey));
    
    //abp
    LoRaMacPayloadEncrypt( (uint8_t *)&loraKeys->abp.devaddr, sizeof(loraKeys->abp.devaddr), key, enc_devaddr, enc_dir, enc_fcnt, encBuffer );
    memcpy((uint8_t *)&loraKeys->abp.devaddr, encBuffer, sizeof(loraKeys->abp.devaddr));
    LoRaMacPayloadEncrypt( loraKeys->abp.nwkskey, sizeof(loraKeys->abp.nwkskey), key, enc_devaddr, enc_dir, enc_fcnt, encBuffer );
    memcpy(loraKeys->abp.nwkskey, encBuffer, sizeof(loraKeys->abp.nwkskey));
    LoRaMacPayloadEncrypt( loraKeys->abp.appskey, sizeof(loraKeys->abp.appskey), key, enc_devaddr, enc_dir, enc_fcnt, encBuffer );
    memcpy(loraKeys->abp.appskey, encBuffer, sizeof(loraKeys->abp.appskey));
    
    return LWAN_SUCCESS;
}

int decrypt_lwan_dev_keys(LWanDevKeys_t *loraKeys)
{
    return encrypt_lwan_dev_keys(loraKeys, loraKeys->pkey);
}

void lwan_mac_params_update()
{
    MibRequestConfirm_t mibReq;
    
    mibReq.Type = MIB_ADR;
    mibReq.Param.AdrEnable = g_lwan_mac_config.modes.adr_enabled;
    LoRaMacMibSetRequestConfirm(&mibReq);
    
    mibReq.Type = MIB_CHANNELS_NB_REP;
    mibReq.Param.ChannelNbRep = g_lwan_mac_config.nbtrials.unconf + 1;
    LoRaMacMibSetRequestConfirm(&mibReq);
    
    mibReq.Type = MIB_CHANNELS_TX_POWER;
    mibReq.Param.ChannelsTxPower = g_lwan_mac_config.tx_power;
    LoRaMacMibSetRequestConfirm(&mibReq);
    
    mibReq.Type = MIB_CHANNELS_DATARATE;
    mibReq.Param.ChannelsDatarate= g_lwan_mac_config.datarate;
    LoRaMacMibSetRequestConfirm(&mibReq);
    
    if(g_lwan_mac_config.rx1_delay) {
        mibReq.Type = MIB_RECEIVE_DELAY_1;
        mibReq.Param.ReceiveDelay1 = g_lwan_mac_config.rx1_delay * 1000;
        LoRaMacMibSetRequestConfirm(&mibReq);
    }
    
    if(g_lwan_mac_config.rx_params.rx2_freq) {                    
        mibReq.Type = MIB_RX2_CHANNEL;
        mibReq.Param.Rx2Channel.Frequency = g_lwan_mac_config.rx_params.rx2_freq;
        mibReq.Param.Rx2Channel.Datarate = g_lwan_mac_config.rx_params.rx2_dr;
        LoRaMacMibSetRequestConfirm(&mibReq);
        
        mibReq.Type = MIB_RX1_DATARATE_OFFSET;
        mibReq.Param.Rx1DrOffset = g_lwan_mac_config.rx_params.rx1_dr_offset;
        LoRaMacMibSetRequestConfirm(&mibReq);
    }
}

void lwan_dev_params_update()
{
    MibRequestConfirm_t mibReq;
     
    uint16_t channelsMaskTemp[8] = {0};
    for (uint8_t i = 0; i < 16; i++) {
        if ((g_lwan_dev_config.freqband_mask & (1 << i)) != 0) {
            channelsMaskTemp[i / 2] |= (0xFF << ((i % 2) * 8));
        }
    }
	if (g_lwan_prodct_config.protl == 1)
	{
        channelsMaskTemp[0] |= 0XFF00;
    }
    mibReq.Type = MIB_CHANNELS_MASK;
    mibReq.Param.ChannelsMask = channelsMaskTemp;
    LoRaMacMibSetRequestConfirm(&mibReq);

    mibReq.Type = MIB_DEVICE_CLASS;
    mibReq.Param.Class = g_lwan_dev_config.modes.class_mode;
    LoRaMacMibSetRequestConfirm(&mibReq);
}

// for LinkWan AT
bool lwan_is_key_valid(uint8_t *key,  uint8_t size)
{
    if(size>LORA_KEY_LENGTH) return false;
    
    uint8_t array_empty[LORA_KEY_LENGTH];
    memset(array_empty, 0, sizeof(array_empty));
    
    return memcmp(key, array_empty, size)?true:false;
}

LWanDevKeys_t *lwan_dev_keys_init(LWanDevKeys_t *default_keys)
{
    if(read_lwan_dev_keys(&g_lwan_dev_keys) != LWAN_SUCCESS) {
        memcpy(&g_lwan_dev_keys, default_keys, sizeof(LWanDevKeys_t));
    }
    if(lwan_is_key_valid(g_lwan_dev_keys.pkey, LORA_KEY_LENGTH))
        decrypt_lwan_dev_keys(&g_lwan_dev_keys);

    return &g_lwan_dev_keys;
}

int lwan_dev_keys_set(int type, void *data)
{
    LWanDevKeys_t keys_saved;
    int ret = LWAN_SUCCESS;
    if(!data) 
        return LWAN_ERROR;

    ret = read_lwan_dev_keys(&keys_saved);
    if((ret == LWAN_SUCCESS) && lwan_is_key_valid(keys_saved.pkey, LORA_KEY_LENGTH)) {
        LOG_PRINTF(LL_WARN, "Lora keys have been protected\r\n");
        return LWAN_ERROR;
    }
    
    switch(type) {
        case DEV_KEYS_OTA_DEVEUI: memcpy(g_lwan_dev_keys.ota.deveui, data, LORA_EUI_LENGTH); break;
        case DEV_KEYS_OTA_APPEUI: memcpy(g_lwan_dev_keys.ota.appeui, data, LORA_EUI_LENGTH); break;
        case DEV_KEYS_OTA_APPKEY: memcpy(g_lwan_dev_keys.ota.appkey, data, LORA_KEY_LENGTH); break;
        case DEV_KEYS_ABP_DEVADDR:  g_lwan_dev_keys.abp.devaddr = *(uint32_t *)data; break;
        case DEV_KEYS_ABP_NWKSKEY: memcpy(g_lwan_dev_keys.abp.nwkskey, data, LORA_KEY_LENGTH); break;
        case DEV_KEYS_ABP_APPSKEY: memcpy(g_lwan_dev_keys.abp.appskey, data, LORA_KEY_LENGTH); break;
        case DEV_KEYS_PKEY: memcpy(g_lwan_dev_keys.pkey, data, LORA_KEY_LENGTH); break;
        default: return LWAN_ERROR; 
    }
    
    memcpy(&keys_saved, &g_lwan_dev_keys, sizeof(keys_saved));
    if(lwan_is_key_valid(keys_saved.pkey, LORA_KEY_LENGTH)) {
        encrypt_lwan_dev_keys(&keys_saved, keys_saved.pkey);
    }
    
    return write_lwan_dev_keys(&keys_saved);
}

int lwan_dev_keys_get(int type, void *data)
{
    LWanDevKeys_t keys;
    int ret = LWAN_SUCCESS;
    if(!data) 
        return LWAN_ERROR;
    
    if(read_lwan_dev_keys(&keys) != 0) {
        memcpy(&keys, &g_lwan_dev_keys, sizeof(keys));
    }
    
    switch(type) {
        case DEV_KEYS_OTA_DEVEUI: memcpy(data, keys.ota.deveui, LORA_EUI_LENGTH); break;
        case DEV_KEYS_OTA_APPEUI: memcpy(data, keys.ota.appeui, LORA_EUI_LENGTH); break;
        case DEV_KEYS_OTA_APPKEY: memcpy(data, keys.ota.appkey, LORA_KEY_LENGTH); break;
        case DEV_KEYS_ABP_DEVADDR: *(uint32_t *)data = keys.abp.devaddr; break;
        case DEV_KEYS_ABP_NWKSKEY: memcpy(data, keys.abp.nwkskey, LORA_KEY_LENGTH); break;
        case DEV_KEYS_ABP_APPSKEY: memcpy(data, keys.abp.appskey, LORA_KEY_LENGTH); break;
        case DEV_KEYS_PKEY: memcpy(data, keys.pkey, LORA_KEY_LENGTH); break;
        default: ret = LWAN_ERROR; break;  
    }
    
    return ret;
}

LWanDevConfig_t *lwan_dev_config_init(LWanDevConfig_t *default_config)
{
    if(read_lwan_dev_config(&g_lwan_dev_config) != LWAN_SUCCESS) {
        memcpy(&g_lwan_dev_config, default_config, sizeof(LWanDevConfig_t));
    }
    return &g_lwan_dev_config;
}

LWanProdctConfig_t *lwan_prodct_config_init(LWanProdctConfig_t *default_config)
{
    memcpy(&g_lwan_prodct_config, default_config, sizeof(LWanProdctConfig_t));

    return &g_lwan_prodct_config;
}

int lwan_dev_config_get(int type, void *config)
{
    int ret = LWAN_SUCCESS;
    
    switch(type) {
        case DEV_CONFIG_JOIN_MODE: {
            *(uint8_t*)config = g_lwan_dev_config.modes.join_mode;
            break;
        }
        case DEV_CONFIG_FREQBAND_MASK: {
            *(uint16_t*)config = g_lwan_dev_config.freqband_mask;
            break;
        }
        case DEV_CONFIG_ULDL_MODE: {
            *(uint8_t*)config = g_lwan_dev_config.modes.uldl_mode;
            break;
        }
        case DEV_CONFIG_WORK_MODE: {
            *(uint8_t*)config = g_lwan_dev_config.modes.work_mode;
            break;
        }
        case DEV_CONFIG_CLASS: {
            *(uint8_t*)config = g_lwan_dev_config.modes.class_mode;
            break;
        }
        case DEV_CONFIG_CLASSB_PARAM: {
            memcpy(config, &g_lwan_dev_config.classb_param, sizeof(g_lwan_dev_config.classb_param));
            break;
        }
        case DEV_CONFIG_JOIN_SETTINGS: {
            memcpy(config, &g_lwan_dev_config.join_settings, sizeof(g_lwan_dev_config.join_settings));
            break;
        }
        default: {
            ret = LWAN_ERROR;
            break;
        }
    }
    
    return ret;
}
int lwan_dev_config_set(int type, void *config)
{
    int ret = LWAN_SUCCESS;
    MibRequestConfirm_t mibReq;
    
    switch(type) {
        case DEV_CONFIG_JOIN_MODE: {
            uint8_t join_mode = *(uint8_t* )config;
            if(join_mode!=JOIN_MODE_ABP && join_mode!=JOIN_MODE_OTAA) 
                return LWAN_ERROR;
            
            if(join_mode==JOIN_MODE_ABP) {
                mibReq.Type = MIB_NET_ID;
                mibReq.Param.NetID = LORAWAN_NETWORK_ID;
                LoRaMacMibSetRequestConfirm(&mibReq);

                mibReq.Type = MIB_DEV_ADDR;
                mibReq.Param.DevAddr = g_lwan_dev_keys.abp.devaddr;
                LoRaMacMibSetRequestConfirm(&mibReq);

                mibReq.Type = MIB_NWK_SKEY;
                mibReq.Param.NwkSKey = g_lwan_dev_keys.abp.nwkskey;
                LoRaMacMibSetRequestConfirm(&mibReq);

                mibReq.Type = MIB_APP_SKEY;
                mibReq.Param.AppSKey = g_lwan_dev_keys.abp.appskey;
                LoRaMacMibSetRequestConfirm(&mibReq);
                if (g_lwan_prodct_config.protl == 1)
                {
#ifdef CONFIG_LINKWAN                    
                    mibReq.Type = MIB_FREQ_BAND;
                    mibReq.Param.freqband = get_next_freqband();
                    LoRaMacMibSetRequestConfirm(&mibReq);
#endif                    
            	}                

                mibReq.Type = MIB_NETWORK_JOINED;
                mibReq.Param.IsNetworkJoined = true;
                LoRaMacMibSetRequestConfirm(&mibReq);
                
                lwan_mac_params_update();
            }
            
            g_lwan_dev_config.modes.join_mode = join_mode;
            break;
        }
        case DEV_CONFIG_FREQBAND_MASK: {
            uint16_t mask = *(uint16_t* )config;
            uint16_t channels_mask[8] = {0};
            for (uint8_t i = 0; i < 16; i++) {
                if ((mask & (1 << i)) != 0) {
                    channels_mask[i / 2] |= (0xFF << ((i % 2) * 8));
                }
            }
            if (g_lwan_prodct_config.protl == 1)
            {
                channels_mask[0] |= 0XFF00;
            }

            mibReq.Type = MIB_CHANNELS_MASK;
            mibReq.Param.ChannelsMask = channels_mask;
            LoRaMacMibSetRequestConfirm(&mibReq);
            
            g_lwan_dev_config.freqband_mask = mask;
            break;
        }
        case DEV_CONFIG_ULDL_MODE: {
            uint8_t uldl_mode = *(uint8_t* )config;
            if(uldl_mode!=ULDL_MODE_INTER && uldl_mode!=ULDL_MODE_INTRA) 
                return LWAN_ERROR;
            
            g_lwan_dev_config.modes.uldl_mode = uldl_mode;
            break;
        }
        case DEV_CONFIG_WORK_MODE: {
            uint8_t work_mode = *(uint8_t* )config;
            if(work_mode!=WORK_MODE_NORMAL) 
                return LWAN_ERROR;
            
            g_lwan_dev_config.modes.work_mode = work_mode;
            break;
        }
        case DEV_CONFIG_CLASS: {
            uint8_t class_mode = *(uint8_t* )config;
            if(class_mode>CLASS_C) 
                return LWAN_ERROR;
            if(g_lwan_dev_config.modes.class_mode == CLASS_B) {
                LoRaMacClassBStop();
                Radio.Sleep();
            }
            
            mibReq.Type = MIB_DEVICE_CLASS;
            mibReq.Param.Class = class_mode;
            LoRaMacMibSetRequestConfirm(&mibReq);
            
            g_lwan_dev_config.modes.class_mode = class_mode;
            break;
        }
        case DEV_CONFIG_CLASSB_PARAM: {
            ClassBParam_t* classb_param = (ClassBParam_t* )config;
            if(classb_param->periodicity>7 || classb_param->beacon_dr>5 || classb_param->pslot_dr>5)
                return LWAN_ERROR;
            
            memcpy(&g_lwan_dev_config.classb_param, classb_param, sizeof(g_lwan_dev_config.classb_param));
            break;
        }
        case DEV_CONFIG_JOIN_SETTINGS: {          
            memcpy(&g_lwan_dev_config.join_settings, config, sizeof(g_lwan_dev_config.join_settings));
            break;
        }
        default: {
            ret = LWAN_ERROR;
            break;
        }
    }
    
    if(ret == LWAN_SUCCESS)
        ret = write_lwan_dev_config(&g_lwan_dev_config);
    
    return ret;
}

LWanMacConfig_t *lwan_mac_config_init(LWanMacConfig_t *default_config)
{
    if(read_lwan_mac_config(&g_lwan_mac_config) != LWAN_SUCCESS) {
        memcpy(&g_lwan_mac_config, default_config, sizeof(LWanMacConfig_t));
    }
    return &g_lwan_mac_config;
}

int lwan_mac_config_get(int type, void *config)
{
    int ret = LWAN_SUCCESS;
    MibRequestConfirm_t mibReq;
    
    switch(type) {
        case MAC_CONFIG_CONFIRM_MSG: {
            *(uint8_t*)config = g_lwan_mac_config.modes.confirmed_msg;
            break;
        }
        case MAC_CONFIG_APP_PORT: {
            *(uint8_t*)config = g_lwan_mac_config.port;
            break;
        }
        case MAC_CONFIG_DATARATE: {
            mibReq.Type = MIB_CHANNELS_DATARATE;
            LoRaMacMibGetRequestConfirm(&mibReq);
            *(uint8_t*)config = mibReq.Param.ChannelsDatarate;
            break;
        }
        case MAC_CONFIG_CONF_NBTRIALS: {
            *(uint8_t*)config = g_lwan_mac_config.nbtrials.conf+1;
            break;
        }
        case MAC_CONFIG_UNCONF_NBTRIALS: {
            *(uint8_t*)config = g_lwan_mac_config.nbtrials.unconf+1;
            break;
        }
        case MAC_CONFIG_REPORT_MODE: {
            *(uint8_t*)config = g_lwan_mac_config.modes.report_mode;
            break;
        }
        case MAC_CONFIG_REPORT_INTERVAL: {
            *(uint32_t*)config = g_lwan_mac_config.report_interval;
            break;
        }
        case MAC_CONFIG_TX_POWER: {
            mibReq.Type = MIB_CHANNELS_TX_POWER;
            LoRaMacMibGetRequestConfirm(&mibReq);
            *(uint8_t*)config = mibReq.Param.ChannelsTxPower;
            break;
        }
        case MAC_CONFIG_CHECK_MODE: {
            *(uint8_t*)config = g_lwan_mac_config.modes.linkcheck_mode;
            break;
        }
        case MAC_CONFIG_ADR_ENABLE: {
            *(uint8_t*)config = g_lwan_mac_config.modes.adr_enabled;
            break;
        }
        case MAC_CONFIG_RX_PARAM: {
            RxParams_t rx_params;
            mibReq.Type = MIB_RX2_CHANNEL;
            LoRaMacMibGetRequestConfirm(&mibReq);
            rx_params.rx2_dr = mibReq.Param.Rx2Channel.Datarate;
            rx_params.rx2_freq = mibReq.Param.Rx2Channel.Frequency;
            mibReq.Type = MIB_RX1_DATARATE_OFFSET;
            LoRaMacMibGetRequestConfirm(&mibReq);
            rx_params.rx1_dr_offset = mibReq.Param.Rx1DrOffset;
            memcpy(config, &rx_params, sizeof(rx_params));
            break;
        }
        case MAC_CONFIG_RX1_DELAY: {
            mibReq.Type = MIB_RECEIVE_DELAY_1;
            LoRaMacMibGetRequestConfirm(&mibReq);
            *(uint16_t*)config = mibReq.Param.ReceiveDelay1/1000;
            break;
        }
        default: {
            ret = LWAN_ERROR;
            break;
        }
    }
    
    return ret;
}
int lwan_mac_config_set(int type, void *config)
{
    int ret = LWAN_SUCCESS;
    MibRequestConfirm_t mibReq;
    
    switch(type) {
        case MAC_CONFIG_CONFIRM_MSG: {
            g_lwan_mac_config.modes.confirmed_msg = *(uint8_t* )config;
            break;
        }
        case MAC_CONFIG_APP_PORT: {
            g_lwan_mac_config.port = *(uint8_t* )config;
            break;
        }
        case MAC_CONFIG_DATARATE: {
            uint8_t dr = *(uint8_t* )config;
            
            mibReq.Type = MIB_CHANNELS_DATARATE;
            mibReq.Param.ChannelsDatarate = dr;
            LoRaMacMibSetRequestConfirm(&mibReq);
            
            g_lwan_mac_config.datarate = dr;
            break;
        }
        case MAC_CONFIG_CONF_NBTRIALS: {
            uint8_t trials = *(uint8_t* )config - 1;
            if(trials>15)
                return LWAN_ERROR;
            
            g_lwan_mac_config.nbtrials.conf = trials;
            break;
        }
        case MAC_CONFIG_UNCONF_NBTRIALS: {
            uint8_t trials = *(uint8_t* )config - 1;
            if(trials>15)
                return LWAN_ERROR;
            
            mibReq.Type = MIB_CHANNELS_NB_REP;
            mibReq.Param.ChannelNbRep = trials+1;
            LoRaMacMibSetRequestConfirm(&mibReq);
    
            g_lwan_mac_config.nbtrials.unconf = trials;
            break;
        }
        case MAC_CONFIG_REPORT_MODE: {
            g_lwan_mac_config.modes.report_mode = *(uint8_t* )config;
            break;
        }
        case MAC_CONFIG_REPORT_INTERVAL: {
            g_lwan_mac_config.report_interval = *(uint32_t* )config;
            if (g_lwan_mac_config.modes.report_mode && g_lwan_mac_config.report_interval) {
                lwan_dev_state_set(DEVICE_STATE_SEND);
            } else {
                g_lwan_mac_config.report_interval = 0;
                g_lwan_mac_config.modes.report_mode = TX_ON_NONE;
                lwan_dev_state_set(DEVICE_STATE_SLEEP);
            }
            break;
        }
        case MAC_CONFIG_TX_POWER: {
            uint8_t power = *(uint8_t* )config;
            
            mibReq.Type = MIB_CHANNELS_TX_POWER;
            mibReq.Param.ChannelsTxPower = power;
            LoRaMacMibSetRequestConfirm(&mibReq);
            
            g_lwan_mac_config.tx_power = power;
            break;
        }
        case MAC_CONFIG_CHECK_MODE: {
            uint8_t check_mode = *(uint8_t* )config;
            if(check_mode>2) 
                return LWAN_ERROR;
            g_lwan_mac_config.modes.linkcheck_mode = check_mode;
            break;
        }
        case MAC_CONFIG_ADR_ENABLE: {
            g_lwan_mac_config.modes.adr_enabled = *(uint8_t* )config;
            
            mibReq.Type = MIB_ADR;
            mibReq.Param.AdrEnable = g_lwan_mac_config.modes.adr_enabled;
            LoRaMacMibSetRequestConfirm(&mibReq);
            break;
        }
        case MAC_CONFIG_RX_PARAM: {
            int status;
            RxParams_t* rx_params = (RxParams_t* )config;
                    
            mibReq.Type = MIB_RX2_CHANNEL;
            mibReq.Param.Rx2Channel.Frequency = rx_params->rx2_freq;
            mibReq.Param.Rx2Channel.Datarate = rx_params->rx2_dr;
            status = LoRaMacMibSetRequestConfirm(&mibReq);
            if(status != LORAMAC_STATUS_OK) {
                return LWAN_ERROR;
            }
            
            mibReq.Type = MIB_RX1_DATARATE_OFFSET;
            mibReq.Param.Rx1DrOffset = rx_params->rx1_dr_offset;
            status = LoRaMacMibSetRequestConfirm(&mibReq);
            if(status != LORAMAC_STATUS_OK) {
                return LWAN_ERROR;
            }

            memcpy(&g_lwan_mac_config.rx_params, rx_params, sizeof(g_lwan_mac_config.rx_params));
            break;
        }
        case MAC_CONFIG_RX1_DELAY: {
            g_lwan_mac_config.rx1_delay = *(uint8_t* )config;
            
            mibReq.Type = MIB_RECEIVE_DELAY_1;
            mibReq.Param.ReceiveDelay1 = g_lwan_mac_config.rx1_delay * 1000;
            LoRaMacMibSetRequestConfirm(&mibReq);
            break;
        }
        default: {
            ret = LWAN_ERROR;
            break;
        }
    }
    
    return ret;
}

int lwan_mac_config_save()
{
    return write_lwan_mac_config(&g_lwan_mac_config);
}

int lwan_mac_config_reset(LWanMacConfig_t *default_config)
{
    int ret = LWAN_SUCCESS;
    
    memcpy(&g_lwan_mac_config, default_config, sizeof(LWanMacConfig_t));
    ret = write_lwan_mac_config(&g_lwan_mac_config);
    if(ret == LWAN_SUCCESS)
        lwan_mac_params_update();
        
    return ret;
}

LWanSysConfig_t *lwan_sys_config_init(LWanSysConfig_t *default_config)
{
    uint16_t crc = 0;
    uint16_t len;
    
    len = sizeof(LWanSysConfig_t);
    read_settings(LWAN_SETTINGS_SYS, &g_lwan_sys_config, len);
    crc = crc16((uint8_t *)&g_lwan_sys_config, len-2);
    if (crc != g_lwan_sys_config.crc){
        memcpy(&g_lwan_sys_config, default_config, sizeof(LWanSysConfig_t));
    }
    
    return &g_lwan_sys_config;
}

int lwan_sys_config_get(int type, void *config)
{
    int ret = LWAN_SUCCESS;
    
    switch(type) {
        case SYS_CONFIG_BAUDRATE: {
            *(uint32_t*)config = g_lwan_sys_config.baudrate;
            break;
        }
        case SYS_CONFIG_LOGLVL: {
            *(uint16_t*)config = g_lwan_sys_config.loglevel;
            break;
        }
        default: {
            ret = LWAN_ERROR;
            break;
        }
    }
    
    return ret;
}
int lwan_sys_config_set(int type, void *config)
{
    int ret = LWAN_SUCCESS;

    switch(type) {
        case SYS_CONFIG_BAUDRATE: {
            g_lwan_sys_config.baudrate = *(uint32_t* )config;
            break;
        }
        case SYS_CONFIG_LOGLVL: {
            g_lwan_sys_config.loglevel = *(uint16_t* )config;
            break;
        }
        default: {
            ret = LWAN_ERROR;
            break;
        }
    }
    
    if(ret == LWAN_SUCCESS){
        g_lwan_sys_config.crc = crc16((uint8_t *)&g_lwan_sys_config, sizeof(LWanSysConfig_t) - 2);
        ret = write_settings(LWAN_SETTINGS_SYS, &g_lwan_sys_config, sizeof(LWanSysConfig_t));
    }
    
    return ret;
}
