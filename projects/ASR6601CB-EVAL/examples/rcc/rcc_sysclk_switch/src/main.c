#include <stdio.h>
#include "tremo_gpio.h"
#include "tremo_rcc.h"

void mco_clk_output()
{
    gpio_set_iomux(GPIOA, GPIO_PIN_5, 5);

    rcc_set_mco_clk_source(RCC_MCO_CLK_SOURCE_SYSCLK);
    rcc_enable_mco_clk_output(true);
}

int main(void)
{
    rcc_enable_oscillator(RCC_OSC_XO32M, true);

    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);

    rcc_set_sys_clk_source(RCC_SYS_CLK_SOURCE_XO32M);

    mco_clk_output();
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
