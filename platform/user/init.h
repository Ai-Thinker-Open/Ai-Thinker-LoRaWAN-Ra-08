#ifndef __USER_INIT_H
#define __USER_INIT_H


#include <stdio.h>

#include "tremo_rcc.h"
#include "tremo_gpio.h"
#include "tremo_uart.h"
#include "tremo_regs.h"

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

void board_init(const rcc_peripheral_t *peripheral_array, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        rcc_enable_peripheral_clk(peripheral_array[i], true);
    }
}

#endif