#include <stdio.h>
#include <string.h>
#include "delay.h"
#include "timer.h"
#include "radio.h"
#include "log.h"
#include "tremo_uart.h"
#include "tremo_lpuart.h"
#include "tremo_gpio.h"
#include "tremo_rcc.h"
#include "tremo_delay.h"
#include "tremo_pwr.h"
#include "rtc-board.h"
#include "linkwan_ica_at.h"
#include "linkwan.h"
#include "lwan_config.h"

#define LORAWAN_APP_PORT 100

void LoraTxData(lora_AppData_t* AppData);
void LoraRxData(lora_AppData_t* AppData);
uint8_t BoardGetBatteryLevel();
void BoardGetUniqueId(uint8_t* id);
uint32_t BoardGetRandomSeed();

log_level_t g_log_level = LL_ERR | LL_WARN | LL_DEBUG;
static LoRaMainCallback_t LoRaMainCallbacks
    = { BoardGetBatteryLevel, BoardGetUniqueId, BoardGetRandomSeed, LoraTxData, LoraRxData };

void uart_log_init(uint32_t baudrate)
{
    lpuart_init_t lpuart_init_cofig;
    uart_config_t uart_config;

    // set iomux
    gpio_set_iomux(GPIOD, GPIO_PIN_12, 2); // LPUART_RX:GP60
    gpio_set_iomux(GPIOB, GPIO_PIN_1, 1);  // UART0_TX:GP17

    // lpuart init
    lpuart_deinit(LPUART);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_LPUART, true);
    lpuart_init_cofig.baudrate         = baudrate;
    lpuart_init_cofig.data_width       = LPUART_DATA_8BIT;
    lpuart_init_cofig.parity           = LPUART_PARITY_NONE;
    lpuart_init_cofig.stop_bits        = LPUART_STOP_1BIT;
    lpuart_init_cofig.low_level_wakeup = false;
    lpuart_init_cofig.start_wakeup     = false;
    lpuart_init_cofig.rx_done_wakeup   = true;
    lpuart_init(LPUART, &lpuart_init_cofig);

    lpuart_config_interrupt(LPUART, LPUART_CR1_RX_DONE_INT, ENABLE);
    lpuart_config_tx(LPUART, false);
    lpuart_config_rx(LPUART, true);

    NVIC_SetPriority(LPUART_IRQn, 2);
    NVIC_EnableIRQ(LPUART_IRQn);

    // uart init
    uart_config_init(&uart_config);
    uart_config.fifo_mode = ENABLE;
    uart_config.mode     = UART_MODE_TX;
    uart_config.baudrate = baudrate;
    uart_init(CONFIG_DEBUG_UART, &uart_config);

    uart_cmd(CONFIG_DEBUG_UART, ENABLE);
}

void board_init()
{
    rcc_enable_oscillator(RCC_OSC_XO32K, true);

    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOB, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOD, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_LPUART, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_UART0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_PWR, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_RTC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SAC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_LORA, true);
    #ifdef PRINT_BY_DMA
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SYSCFG, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_DMA0, true);
    #endif

    delay_ms(100);
    pwr_xo32k_lpm_cmd(true);

    RtcInit();
}

void* _sbrk(int nbytes)
{
    // variables from linker script
    extern char _heap_bottom[];
    extern char _heap_top[];

    static char* heap_ptr = _heap_bottom;

    if ((unsigned int)(heap_ptr + nbytes) <= (unsigned int)_heap_top) {
        void* base = heap_ptr;
        heap_ptr += nbytes;
        return base;
    } else {
        return (void*)(-1);
    }
}

uint8_t BoardGetBatteryLevel()
{
    return 0;
}

void BoardGetUniqueId(uint8_t* id)
{
}

uint32_t BoardGetRandomSeed()
{
    return 0;
}

void LoraTxData(lora_AppData_t* AppData)
{
    AppData->BuffSize = sprintf((char*)AppData->Buff, "linklora asr data++");
    LOG_PRINTF(LL_DEBUG, "tx: %s\r\n", AppData->Buff);
    AppData->Port = LORAWAN_APP_PORT;
}

void LoraRxData(lora_AppData_t* AppData)
{
    AppData->Buff[AppData->BuffSize] = '\0';
    LOG_PRINTF(LL_DEBUG, "rx: port = %d, len = %d\r\n", AppData->Port, AppData->BuffSize);
    int i;
    for (i = 0; i < AppData->BuffSize; i++) {
        LOG_PRINTF(LL_DEBUG, "0x%x ", AppData->Buff[i]);
    }
    LOG_PRINTF(LL_DEBUG, "\r\n");
}

int main(void)
{
    LWanSysConfig_t default_sys_config = LWAN_SYS_CONFIG_DEFAULT;
    uint32_t baudrate;
    uint16_t log_level;

    // Target board initialization
    board_init();

    lwan_sys_config_init(&default_sys_config);
    lwan_sys_config_get(SYS_CONFIG_BAUDRATE, &baudrate);
    uart_log_init(baudrate);

    lwan_sys_config_get(SYS_CONFIG_LOGLVL, &log_level);
    log_set_level((1<<log_level)-1);

    lora_init(&LoRaMainCallbacks);
    lora_fsm();
}

#ifdef USE_FULL_ASSERT
void assert_failed(void* file, uint32_t line)
{
    (void)file;
    (void)line;

    while (1) { }
}
#endif
