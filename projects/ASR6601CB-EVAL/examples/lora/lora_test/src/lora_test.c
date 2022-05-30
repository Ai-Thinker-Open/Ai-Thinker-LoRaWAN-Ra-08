#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "tremo_uart.h"
#include "tremo_gpio.h"
#include "tremo_pwr.h"
#include "delay.h"
#include "timer.h"
#include "sx126x-board.h"
#include "sx126x.h"
#include "radio.h"

#define RF_FREQUENCY               470000000
#define TX_OUTPUT_POWER            22        // dBm
#define LORA_BANDWIDTH             0         // [0: 125 kHz,
                                             //  1: 250 kHz,
                                             //  2: 500 kHz,
                                             //  3: Reserved]
#define LORA_SPREADING_FACTOR      12        // [SF7..SF12]
#define LORA_CODINGRATE            1         // [1: 4/5,
                                             //  2: 4/6,
                                             //  3: 4/7,
                                             //  4: 4/8]
#define LORA_PREAMBLE_LENGTH       8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT        5         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON       false

#define AT_PROMPT "ASR6601:~#"

typedef int(ATTestHandler)(int argc, char* argv[]);
typedef struct TestCaseSt_ {
    char name[32];
    ATTestHandler* fn;
} TestCaseSt;

int test_case_ctxcw(int argc, char* argv[]);
int test_case_ctx(int argc, char* argv[]);
int test_case_crx(int argc, char* argv[]);
int test_case_crxs(int argc, char* argv[]);
int test_case_csleep(int argc, char* argv[]);
int test_case_cstdby(int argc, char* argv[]);

static TestCaseSt gCases[] = { 
    { "AT+CTXCW=", &test_case_ctxcw }, 
    { "AT+CTX=", &test_case_ctx }, 
    { "AT+CRXS=", &test_case_crxs },
    { "AT+CRX=", &test_case_crx }, 
    { "AT+CSLEEP=", &test_case_csleep }, 
    { "AT+CSTDBY=", &test_case_cstdby } 
};
static RadioEvents_t TestRadioEvents;
static uint32_t g_fcnt_start = 0;
static uint32_t g_fcnt_rcvd  = 0;
static uint32_t g_fcnt_send  = 1;
static uint32_t g_freq       = RF_FREQUENCY;
static uint8_t g_sf          = LORA_SPREADING_FACTOR;
static uint8_t g_bw          = LORA_BANDWIDTH;
static uint8_t g_cr          = LORA_CODINGRATE;
static uint8_t g_tx_power    = TX_OUTPUT_POWER;
static uint8_t g_tx_len      = 0;
static bool g_lora_test_rxs  = false;
static bool g_lora_tx_done   = false;

gpio_t*  g_test_gpiox = GPIOA;
uint32_t g_test_pin   = GPIO_PIN_11;

TimerTime_t ts_rxdone_pre = 0;

extern SX126x_t SX126x;

void OnTxDone(void)
{
    printf("OnTxDone\r\n");
    g_lora_tx_done = true;
}

void OnRxDone(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr)
{
    int i = 0;
	static uint8_t ledStatus=0;

    printf("OnRxDone\r\n");
	if(ledStatus){
		gpio_init(GPIOA,GPIO_PIN_5, GPIO_MODE_OUTPUT_PP_LOW);
		gpio_init(GPIOA,GPIO_PIN_6, GPIO_MODE_OUTPUT_PP_HIGH);
		ledStatus=0;
	}else{
		gpio_init(GPIOA,GPIO_PIN_5, GPIO_MODE_OUTPUT_PP_HIGH);
		gpio_init(GPIOA,GPIO_PIN_6, GPIO_MODE_OUTPUT_PP_LOW);
		ledStatus=1;
	}
    if (g_lora_test_rxs) {
        TimerTime_t ts_rxdone_now = TimerGetCurrentTime();
        uint32_t delt_rxdone_ts = 0;
        if(ts_rxdone_pre)
            delt_rxdone_ts = ts_rxdone_now - ts_rxdone_pre;
        ts_rxdone_pre = ts_rxdone_now;
		
        g_fcnt_rcvd++;
		
        printf("[%lu]Received, rssi = %d, snr = %d, delt_rxdone_ts: %lu\r\n", g_fcnt_rcvd, rssi, (int16_t)snr, delt_rxdone_ts);
        printf("  ");
        for (i = 0; i < size; i++) {
            printf("0x%x ", payload[i]);
        }
        printf("\r\n");
    } else {
        uint32_t fcnt = strtol((const char*)payload, NULL, 0);
        if (!g_fcnt_start)
            g_fcnt_start = fcnt;
        g_fcnt_rcvd++;

        if (fcnt) {
            printf("[%lu/%lu]Received: %lu, rssi = %d, snr = %d\r\n", g_fcnt_rcvd, (fcnt - g_fcnt_start + 1), fcnt, rssi,
                snr);
        } else {
            printf("Received, size: %u, rssi = %d, snr = %d\r\n", size, rssi, snr);
            printf("  ");
            for (i = 0; i < size; i++) {
                printf("0x%x ", payload[i]);
            }
            printf("\r\n");
        }
    }
}

void OnTxTimeout(void)
{
    printf("OnTxTimeout\r\n");
}

void OnRxTimeout(void)
{
    printf("OnRxTimeout\r\n");
}

void OnRxError(void)
{
    printf("OnRxError\r\n");
}

void test_stop3_wfi()
{
    gpio_init(g_test_gpiox, g_test_pin, GPIO_MODE_INPUT_PULL_UP); 
    gpio_config_interrupt(g_test_gpiox, g_test_pin, GPIO_INTR_FALLING_EDGE);
    gpio_config_stop3_wakeup(g_test_gpiox, g_test_pin, true, GPIO_LEVEL_LOW);
    
    /* NVIC config */
    NVIC_EnableIRQ(GPIO_IRQn);
    NVIC_SetPriority(GPIO_IRQn, 2);
    
    printf("enter deepsleep...\r\n");
    while(uart_get_flag_status(CONFIG_DEBUG_UART, UART_FLAG_BUSY));
    
    gpio_set_iomux(GPIOB, GPIO_PIN_1, 0);
    pwr_deepsleep_wfi(PWR_LP_MODE_STOP3);
    gpio_set_iomux(GPIOB, GPIO_PIN_1, 1);

    printf("leave deepsleep...\r\n");
}

int test_case_ctxcw(int argc, char* argv[])
{
    uint8_t opt   = 0;
    uint32_t freq = strtol(argv[0], NULL, 0);
    uint8_t pwr   = strtol(argv[1], NULL, 0);
    if (argc > 2)
        opt = strtol(argv[2], NULL, 0);

    TestRadioEvents.TxDone    = OnTxDone;
    TestRadioEvents.RxDone    = OnRxDone;
    TestRadioEvents.TxTimeout = OnTxTimeout;
    TestRadioEvents.RxTimeout = OnRxTimeout;
    TestRadioEvents.RxError   = OnRxError;

    SX126xSetPaOpt(opt);
    Radio.Init(&TestRadioEvents);
	SX126xAntSwOff();	//使用双控的时候 AT+CTXCW 控制另一端

    printf("Start to txcw (freq: %lu, power: %udb, opt: %u)\r\n", freq, pwr, opt);
    Radio.SetTxContinuousWave(freq, pwr, 0xffff);

    return 0;
}

int test_case_cstdby(int argc, char* argv[])
{
    uint8_t stdby_mode = 0;
    stdby_mode         = strtol((const char*)argv[0], NULL, 0);

    TestRadioEvents.TxDone    = OnTxDone;
    TestRadioEvents.RxDone    = OnRxDone;
    TestRadioEvents.TxTimeout = OnTxTimeout;
    TestRadioEvents.RxTimeout = OnRxTimeout;
    TestRadioEvents.RxError   = OnRxError;

    Radio.Init(&TestRadioEvents);

    // lora standby
    if (stdby_mode == 0)
        SX126xSetStandby(STDBY_RC);
    else
        SX126xSetStandby(STDBY_XOSC);
    
    test_stop3_wfi();
	
    return 0;
}

int test_case_csleep(int argc, char* argv[])
{
    uint8_t sleep_mode = 0;
    sleep_mode         = strtol((const char*)argv[0], NULL, 0);

    TestRadioEvents.TxDone    = OnTxDone;
    TestRadioEvents.RxDone    = OnRxDone;
    TestRadioEvents.TxTimeout = OnTxTimeout;
    TestRadioEvents.RxTimeout = OnRxTimeout;
    TestRadioEvents.RxError   = OnRxError;

    Radio.Init(&TestRadioEvents);

    // lora sleep
    if (sleep_mode == 0)
        Radio.Sleep();
    else {
        SleepParams_t params    = { 0 };
        params.Fields.WarmStart = 0;
        SX126xSetSleep(params);
    }
    
    test_stop3_wfi();

    return 0;
}

int test_case_crxs(int argc, char* argv[])
{
    uint32_t freq = strtol(argv[0], NULL, 0);
    uint8_t dr    = strtol(argv[1], NULL, 0);
    uint8_t bw    = strtol(argv[2], NULL, 0);
    uint8_t cr    = strtol(argv[3], NULL, 0);
    uint8_t ldo   = strtol(argv[4], NULL, 0);

    g_freq = freq;
    g_sf   = 12 - dr;
    g_cr   = cr;
    g_bw   = bw;

    TestRadioEvents.TxDone    = OnTxDone;
    TestRadioEvents.RxDone    = OnRxDone;
    TestRadioEvents.TxTimeout = OnTxTimeout;
    TestRadioEvents.RxTimeout = OnRxTimeout;
    TestRadioEvents.RxError   = OnRxError;
    Radio.Init(&TestRadioEvents);

    Radio.SetChannel(g_freq);
    Radio.SetRxConfig(MODEM_LORA, g_bw, g_sf, g_cr, 0, LORA_PREAMBLE_LENGTH, LORA_SYMBOL_TIMEOUT,
        LORA_FIX_LENGTH_PAYLOAD_ON, 0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

    SX126x.ModulationParams.Params.LoRa.LowDatarateOptimize = ldo;
    SX126xSetModulationParams(&SX126x.ModulationParams);

    g_lora_test_rxs = true;
    Radio.Rx(0);

    printf("start to recv package (freq: %lu, dr:%u, bw: %u, cr: %u, ldo: %u)\r\n", freq, dr, bw, cr, ldo);
    while (1) {
        Radio.IrqProcess();
    }
}

int test_case_crx(int argc, char* argv[])
{
    uint32_t freq = strtol(argv[0], NULL, 0);
    uint8_t dr    = strtol(argv[1], NULL, 0);
    uint8_t bw    = strtol(argv[2], NULL, 0); 
    uint8_t cr    = strtol(argv[3], NULL, 0); 
    
    g_freq = freq;
    g_sf   = 12 - dr;
    g_bw   = bw;
    g_cr   = cr;

    TestRadioEvents.TxDone    = OnTxDone;
    TestRadioEvents.RxDone    = OnRxDone;
    TestRadioEvents.TxTimeout = OnTxTimeout;
    TestRadioEvents.RxTimeout = OnRxTimeout;
    TestRadioEvents.RxError   = OnRxError;

    Radio.Init(&TestRadioEvents);

    Radio.SetChannel(g_freq);
    Radio.SetRxConfig( MODEM_LORA, g_bw, g_sf,
                                       g_cr, 0, LORA_PREAMBLE_LENGTH,
                                       LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                       0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
    g_lora_test_rxs = false;
    Radio.Rx(0);

    printf("start to recv package (freq: %lu, dr:%u, bw: %d, cr: %d)\r\n", freq, dr, g_bw, g_cr);

    while (1) {
        Radio.IrqProcess();
    }
}

int test_case_ctx(int argc, char* argv[])
{
    char buf[32];
    uint32_t size = 0;
    uint8_t len   = 0;
    uint32_t freq = strtol(argv[0], NULL, 0);
    uint8_t dr    = strtol(argv[1], NULL, 0);
    uint8_t bw    = strtol(argv[2], NULL, 0);  
    uint8_t cr    = strtol(argv[3], NULL, 0);  
    uint8_t pwr   = strtol(argv[4], NULL, 0);
    if(argc>5)
        len = strtol(argv[5], NULL, 0);

    g_freq     = freq;
    g_sf       = 12 - dr;
    g_bw       = bw;
    g_cr       = cr;
    g_tx_power = pwr;
    g_tx_len   = len;

    TestRadioEvents.TxDone    = OnTxDone;
    TestRadioEvents.RxDone    = OnRxDone;
    TestRadioEvents.TxTimeout = OnTxTimeout;
    TestRadioEvents.RxTimeout = OnRxTimeout;
    TestRadioEvents.RxError   = OnRxError;
    Radio.Init(&TestRadioEvents);

    size = sprintf(buf, "%lu", g_fcnt_send);

    printf("start to tx data(freq: %lu, dr: %u, bw:%d, cr: %d, power: %u): %lu\r\n", g_freq, 12-g_sf, g_bw, g_cr, g_tx_power, g_fcnt_send);
    Radio.SetChannel(g_freq);
    Radio.SetTxConfig( MODEM_LORA, g_tx_power, 0, g_bw,
                           g_sf, g_cr,
                           LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                           true, 0, 0, LORA_IQ_INVERSION_ON, 60000 );

    Radio.Send((uint8_t*)buf, size > g_tx_len ? size : g_tx_len);
    g_fcnt_send++;

    while (1) {
        Radio.IrqProcess();

        if (g_lora_tx_done) {
            DelayMs(1000);

            size = sprintf(buf, "%lu", g_fcnt_send);
            printf("start to tx data(freq: %lu, dr: %u, bw:%d, cr: %d, power: %u): %lu\r\n", g_freq, 12-g_sf, g_bw, g_cr, g_tx_power, g_fcnt_send);
            Radio.Send((uint8_t*)buf, size > g_tx_len ? size : g_tx_len);
            g_fcnt_send++;
            g_lora_tx_done = false;
        }
    }
}

int tc_lora_test(void)
{
    int ret   = -1;
    int i     = 0;
    char* ptr = NULL;
    char ch;
    char cmd_str[64];
    int cmd_index = 0;
    int case_num  = sizeof(gCases) / sizeof(gCases[0]);
    int argc      = 0;
    char* argv[16];
    char* str      = NULL;
    char resetFlag = 1;

    while (1) {
        cmd_index = 0;
        argc      = 0;
        if (resetFlag == 1) {
            memset(cmd_str, 0, sizeof(cmd_str));
            resetFlag = 0;
        }
        ch = '\0';
        printf("\r\n%s", AT_PROMPT);
        while (ch == '\0' || (ch != '\r' && ch != '\n')) {
            resetFlag = 1;
            ch        = uart_receive_data(CONFIG_DEBUG_UART);
            if (ch != '\0' && ch != '\r' && ch != '\n')
                cmd_str[cmd_index++] = ch;
        }
        if (cmd_index == 0)
            continue;
        cmd_str[cmd_index] = '\0';

        for (i = 0; i < case_num; i++) {
            int cmd_len = strlen(gCases[i].name);
            if (!strncmp((const char*)cmd_str, gCases[i].name, cmd_len)) {
                ptr = (char*)cmd_str + cmd_len;
                break;
            }
        }

        if (i >= case_num || !gCases[i].fn)
            goto end;

        str = strtok((char*)ptr, ",");
        while (str) {
            argv[argc++] = str;
            str          = strtok((char*)NULL, ",");
        }
        ret = gCases[i].fn(argc, argv);

    end:
        if (ret == -1)
            printf("\r\n+CME ERROR:1\r\n");
    }
}
