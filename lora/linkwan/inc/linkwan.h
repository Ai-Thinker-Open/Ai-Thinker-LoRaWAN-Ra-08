/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#ifndef LINKWAN_H
#define LINKWAN_H

#include <stdbool.h>
#include <stdio.h>
#include "log.h"

#define LORAWAN_APP_DATA_BUFF_SIZE 242
#define LINKWAN_APP_DATA_SIZE 51
#define LORAWAN_CONFIRMED_MSG 1
#define LORAWAN_UNCONFIRMED_MSG 0
#define JOINREQ_NBTRIALS 3    

typedef enum eTxEventType {
    TX_ON_NONE,
    TX_ON_TIMER
} TxEventType_t;

typedef struct {
    uint8_t *Buff;
    uint8_t BuffSize;
    uint8_t Port;
} lora_AppData_t;

typedef struct sLoRaMainCallback {
    uint8_t (*BoardGetBatteryLevel)(void);
    void (*BoardGetUniqueId)(uint8_t *id);
    uint32_t (*BoardGetRandomSeed)(void);
    void (*LoraTxData)(lora_AppData_t *AppData);
    void (*LoraRxData)(lora_AppData_t *AppData);
} LoRaMainCallback_t;

typedef enum eDevicState {
    DEVICE_STATE_INIT,
    DEVICE_STATE_JOIN,
    DEVICE_STATE_JOINED,
    DEVICE_STATE_SEND,
    DEVICE_STATE_SEND_MAC,
    DEVICE_STATE_CYCLE,
    DEVICE_STATE_SLEEP,
    DEVICE_STATE_REQ_DEVICE_TIME,
    DEVICE_STATE_BEACON_ACQUISITION,
    DEVICE_STATE_REQ_PINGSLOT_ACK
} DeviceState_t;

typedef enum eDeviceStatus {
    DEVICE_STATUS_IDLE,//0
    DEVICE_STATUS_SENDING,
    DEVICE_STATUS_SEND_FAIL,
    DEVICE_STATUS_SEND_PASS,
    DEVICE_STATUS_JOIN_PASS,
    DEVICE_STATUS_JOIN_FAIL,
    DEVICE_STATUS_NETWORK_ABNORMAL,
    DEVICE_STATUS_SEND_PASS_WITHOUT_DL,
    DEVICE_STATUS_SEND_PASS_WITH_DL,
} DeviceStatus_t;

typedef enum eMacReq {
    MAC_REQ_LINKCHECK,//0
    MAC_REQ_DEVICE_TIME,
    MAC_REQ_PSLOT_INFO
} MacReq_t;

void lora_init(LoRaMainCallback_t *callbacks);
void lora_fsm( void );

bool lwan_is_dev_busy();
DeviceState_t lwan_dev_state_get( void );
void lwan_dev_state_set(DeviceState_t state);
DeviceStatus_t lwan_dev_status_get(void);
bool lwan_dev_status_set(DeviceStatus_t ds);

int lwan_join(uint8_t bJoin, uint8_t bAutoJoin, uint16_t joinInterval, uint16_t joinRetryCnt);
int lwan_mac_req_send(int type, void *param);
int lwan_data_send(uint8_t confirm, uint8_t Nbtrials, uint8_t *payload, uint8_t size);
int lwan_data_recv(uint8_t *port, uint8_t **payload, uint8_t *size);

int lwan_dev_rssi_get(uint8_t band, int16_t *channel_rssi);
uint8_t lwan_dev_battery_get();

bool lwan_multicast_add(void *multicastInfo );
bool lwan_multicast_del(uint32_t dev_addr);
uint8_t lwan_multicast_num_get(void);

void lwan_sys_reboot(int8_t mode);

#endif /* LINKWAN_H */
