#include <stdio.h>
#include "tremo_regs.h"
#include "tremo_lpuart.h"
#include "tremo_rcc.h"
#include "tremo_pwr.h"
#include "tremo_gpio.h"

uint8_t rx_data[11] = { 0 };
uint8_t rx_index    = 0;

void lpuart_rx()
{
    lpuart_init_t lpuart_init_cofig;

    gpio_set_iomux(GPIOC, GPIO_PIN_15, 2); // TX:GP47
    gpio_set_iomux(GPIOD, GPIO_PIN_12, 2); // RX:GP60

    lpuart_init_cofig.baudrate         = 9600;
    lpuart_init_cofig.data_width       = LPUART_DATA_8BIT;
    lpuart_init_cofig.parity           = LPUART_PARITY_NONE;
    lpuart_init_cofig.stop_bits        = LPUART_STOP_1BIT;
    lpuart_init_cofig.low_level_wakeup = true;
    lpuart_init_cofig.start_wakeup     = false;
    lpuart_init_cofig.rx_done_wakeup   = false;
    lpuart_init(LPUART, &lpuart_init_cofig);

    lpuart_config_interrupt(LPUART, LPUART_CR1_RX_NOT_EMPTY_INT, ENABLE);

    lpuart_config_rx(LPUART, true);

    /* NVIC config */
    NVIC_SetPriority(LPUART_IRQn, 2);
    NVIC_EnableIRQ(LPUART_IRQn);
}

void lpuart_IRQHandler()
{
    if (lpuart_get_rx_not_empty_status(LPUART)) {
        rx_data[rx_index++] = lpuart_receive_data(LPUART);
    }
}

int main(void)
{
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_AFEC, true);

    // enable the clk
    rcc_enable_oscillator(RCC_OSC_XO32K, true);

    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOB, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOD, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_PWR, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_LPUART, true);

    lpuart_rx();

    pwr_deepsleep_wfi(PWR_LP_MODE_STOP3);

    /* Infinite loop */
    while (1) { }
}

#ifdef USE_FULL_ASSERT
void assert_failed(void* file, uint32_t line)
{
    (void)file;
    (void)line;

    while (1) { }
}
#endif
