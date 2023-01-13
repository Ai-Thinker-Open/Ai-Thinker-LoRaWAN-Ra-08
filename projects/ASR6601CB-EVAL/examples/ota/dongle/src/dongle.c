/*!
 * \file      main.c
 *
 * \brief     Ping-Pong implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "delay.h"
#include "timer.h"
#include "radio.h"
#include "at_command.h"


#define RF_FREQUENCY                    470000000
#define TX_OUTPUT_POWER                             22        // dBm
#define LORA_BANDWIDTH                              2         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         5         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

#define FSK_FIX_LENGTH_PAYLOAD_ON                   false

//global variables
static RadioEvents_t RadioEvents;
static uint32_t g_freq = RF_FREQUENCY;
static uint32_t g_txp = TX_OUTPUT_POWER;
static uint32_t g_modem = MODEM_LORA;
static uint32_t g_lora_bw = LORA_BANDWIDTH;
static uint32_t g_lora_sf = LORA_SPREADING_FACTOR;
static uint32_t g_lora_cr = LORA_CODINGRATE;
static uint32_t g_lora_preamble = LORA_PREAMBLE_LENGTH;
static uint32_t g_lora_iqi = LORA_IQ_INVERSION_ON;
static uint32_t g_fsk_bw = 100000;
static uint32_t g_fsk_dr = 50000;
static uint32_t g_fsk_dev = 25000;
static uint32_t g_fsk_preamble = 5;
static uint32_t g_fsk_afcbw = 166666;

void lora_tx(uint8_t *data, uint32_t size);
void lora_rx(uint32_t timeout);

void OnTxDone( void )
{
    lora_rx( 0 );
    printf("\r\nOK+SEND\r\n");
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    lora_rx( 0 );
    
    printf("\r\nAT+DATA=0,%d,%d,%u,", snr, rssi, size);
    for(int i=0; i<size; i++)
        printf("%02X", payload[i]);
    printf("\r\n");
}

void OnTxTimeout( void )
{
    lora_rx( 0 );
    printf("\r\nERR+SEND:1\r\n");
}

void OnRxTimeout( void )
{
    lora_rx( 0 );
    printf("\r\nAT+DATA=1\r\n");
}

void OnRxError( void )
{
    lora_rx( 0 );
    printf("\r\nAT+DATA=2\r\n");
}

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

int at_freq(int opt, int argc, char *argv[])
{
    if(argc<1)
        return -1;
    
    g_freq = strtol(argv[0], NULL, 0);
    
    Radio.SetChannel(g_freq);

    printf("\r\nOK\r\n");
    return 0;
}

int at_cfg(int opt, int argc, char *argv[])
{
    uint32_t p1, p2, p3, p4, p5;
    if(argc<7)
        return -1;
    
    g_modem = strtol(argv[0], NULL, 0);
    p1 = strtol(argv[1], NULL, 0);
    p2 = strtol(argv[2], NULL, 0);
    p3 = strtol(argv[3], NULL, 0);
    p4 = strtol(argv[4], NULL, 0);
    p5 = strtol(argv[5], NULL, 0);
    g_txp = strtol(argv[6], NULL, 0);
    
    if(g_modem == MODEM_LORA) {
        g_lora_bw = p1;
        g_lora_sf = p2;
        g_lora_cr = p3;
        g_lora_preamble = p4;
        g_lora_iqi = p5;
    }else{
        g_fsk_bw = p1;
        g_fsk_dr = p2;
        g_fsk_dev = p3;
        g_fsk_preamble = p4;
        g_fsk_afcbw = p5;
    }
    
    lora_rx(0);
    printf("\r\nOK\r\n");
    return 0;
}

int at_tx(int opt, int argc, char *argv[])
{
    int len, bin_len;
    uint8_t data[256];
    len = strtol((const char *)argv[0], NULL, 0);
    bin_len = hex2bin((const char *)argv[1], data, len);
    
    if(bin_len>0) {
        lora_tx( data, bin_len );
    }
    
    return 0;
}

int at_rx(int opt, int argc, char *argv[])
{
    uint32_t rx_timeout = strtol(argv[0], NULL, 0);
    
    lora_rx( rx_timeout );
    
    printf("\r\nOK\r\n");
    return 0;
}

void lora_tx(uint8_t *data, uint32_t size)
{
    Radio.Sleep();
    
    Radio.SetChannel(g_freq);
    if(g_modem == MODEM_LORA) {
        
        Radio.SetTxConfig( MODEM_LORA, g_txp, 0, g_lora_bw,
                                       g_lora_sf, g_lora_cr,
                                       g_lora_preamble, LORA_FIX_LENGTH_PAYLOAD_ON,
                                       true, 0, 0, g_lora_iqi, 6000 );
    }else{
        Radio.SetTxConfig( MODEM_FSK, g_txp, g_fsk_dev, 0,
                                      g_fsk_dr, 0,
                                      g_fsk_preamble, FSK_FIX_LENGTH_PAYLOAD_ON,
                                      true, 0, 0, 0, 6000 );
    }
    
    Radio.Send(data, size);
}

void lora_rx(uint32_t timeout)
{
    Radio.Sleep();
    
    if(g_modem == MODEM_LORA) {
        Radio.SetRxConfig( MODEM_LORA, g_lora_bw, g_lora_sf,
                                       g_lora_cr, 0, g_lora_preamble,
                                       LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                       0, true, 0, 0, g_lora_iqi, true );
    }else{
        Radio.SetRxConfig( MODEM_FSK, g_fsk_bw, g_fsk_dr,
                                      0, g_fsk_afcbw, g_fsk_preamble,
                                      0, FSK_FIX_LENGTH_PAYLOAD_ON, 0, true,
                                      0, 0, false, true );
    }
    
    Radio.Rx(timeout);
}

/**
 * Main application entry point.
 */
int app_start( void )
{
    // Radio initialization
    RadioEvents.TxDone = OnTxDone;
    RadioEvents.RxDone = OnRxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxTimeout = OnRxTimeout;
    RadioEvents.RxError = OnRxError;

    Radio.Init( &RadioEvents );
    
    Radio.SetChannel(g_freq);
    
    if(g_modem == MODEM_LORA) {
        
        Radio.SetTxConfig( MODEM_LORA, g_txp, 0, g_lora_bw,
                                       g_lora_sf, g_lora_cr,
                                       g_lora_preamble, LORA_FIX_LENGTH_PAYLOAD_ON,
                                       true, 0, 0, g_lora_iqi, 6000 );
    
        Radio.SetRxConfig( MODEM_LORA, g_lora_bw, g_lora_sf,
                                       g_lora_cr, 0, g_lora_preamble,
                                       LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                       0, true, 0, 0, g_lora_iqi, true );
    }else{
        Radio.SetTxConfig( MODEM_FSK, g_txp, g_fsk_dev, 0,
                                      g_fsk_dr, 0,
                                      g_fsk_preamble, FSK_FIX_LENGTH_PAYLOAD_ON,
                                      true, 0, 0, 0, 6000 );
    
        Radio.SetRxConfig( MODEM_FSK, g_fsk_bw, g_fsk_dr,
                                      0, g_fsk_afcbw, g_fsk_preamble,
                                      0, FSK_FIX_LENGTH_PAYLOAD_ON, 0, true,
                                      0, 0, false, true );
    }

    Radio.Rx( 0 );

    at_init();
    while( 1 ) {
        if( Radio.IrqProcess != NULL ) {
            Radio.IrqProcess( );
        }
        
        at_process();
    }
}
