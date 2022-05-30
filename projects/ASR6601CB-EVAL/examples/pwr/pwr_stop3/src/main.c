#include <stdio.h>
#include "tremo_rcc.h"
#include "tremo_pwr.h"
#include "tremo_delay.h"
#include "tremo_gpio.h"

gpio_t *g_test_gpiox = GPIOA;
uint8_t g_test_pin = GPIO_PIN_11;
volatile uint32_t g_gpio_interrupt_flag = 0;

int main(void)
{
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_PWR, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);

    gpio_init(g_test_gpiox, g_test_pin, GPIO_MODE_INPUT_PULL_UP);
    gpio_config_stop3_wakeup(g_test_gpiox, g_test_pin, true, GPIO_LEVEL_LOW);
    gpio_config_interrupt(g_test_gpiox, g_test_pin, GPIO_INTR_FALLING_EDGE);

    /* NVIC config */
    NVIC_EnableIRQ(GPIO_IRQn);
    NVIC_SetPriority(GPIO_IRQn, 2);

    /* Infinite loop */
    while (1) {

        pwr_deepsleep_wfi(PWR_LP_MODE_STOP3);

        if (g_gpio_interrupt_flag) {
            g_gpio_interrupt_flag = 0;
        }
        delay_ms(2000);
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(void* file, uint32_t line)
{
    (void)file;
    (void)line;

    while (1) { }
}
#endif
