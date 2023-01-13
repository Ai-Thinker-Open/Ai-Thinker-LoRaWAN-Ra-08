#include <stdio.h>
#include <string.h>
#include "delay.h"
#include "timer.h"
#include "radio.h"
#include "tremo_uart.h"
#include "tremo_gpio.h"
#include "tremo_rcc.h"
#include "tremo_pwr.h"
#include "tremo_delay.h"
#include "rtc-board.h"


extern int tc_lora_test(void);

void uart_log_init(void)
{
    // uart0
    gpio_set_iomux(GPIOB, GPIO_PIN_0, 1);
    gpio_set_iomux(GPIOB, GPIO_PIN_1, 1);

    /* uart config struct init */
    uart_config_t uart_config;
    uart_config_init(&uart_config);

    uart_config.baudrate = UART_BAUDRATE_115200;
    uart_init(CONFIG_DEBUG_UART, &uart_config);
    uart_cmd(CONFIG_DEBUG_UART, ENABLE);
}

void board_init()
{
    rcc_enable_oscillator(RCC_OSC_XO32K, true);

    rcc_enable_peripheral_clk(RCC_PERIPHERAL_UART0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOB, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOD, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_PWR, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_RTC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_LORA, true);
    
    delay_ms(100);
    pwr_xo32k_lpm_cmd(true);
    
    uart_log_init();

    RtcInit();
}

int main(void)
{
    // Target board initialization
    board_init();

    printf("\r\n"
            "/*******************************************************************\r\n"
            "********************************************************************\r\n"
            "*                      ASR6601 LoRa Test                           *\r\n"
            "* Available commands are:                                          *\r\n"
            "* AT+CTXCW=<freq>,<pwr>[,opt]                                      *\r\n"
            "* AT+CTX=<freq>,<data_rate>,<bandwidth>,<code_rate>,<pwr>[,tx_len] *\r\n"
            "* AT+CRX=<freq>,<data_rate>,<bandwidth>,<code_rate>                *\r\n"
            "* AT+CRXS=<freq>,<data_rate>,<bandwidth>,<code_rate>,<ldo>         *\r\n"
            "* AT+CSLEEP=<sleep_mode>                                           *\r\n"
            "* AT+CSTDBY=<standby_mode>                                         *\r\n"
            "********************************************************************\r\n"
            "*******************************************************************/\r\n");
	
    tc_lora_test();
}

#ifdef USE_FULL_ASSERT
void assert_failed(void* file, uint32_t line)
{
    (void)file;
    (void)line;

    while (1) { }
}
#endif