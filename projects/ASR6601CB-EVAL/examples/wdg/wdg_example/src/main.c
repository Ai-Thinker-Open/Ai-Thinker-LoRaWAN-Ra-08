#include <stdio.h>
#include "tremo_rcc.h"
#include "tremo_wdg.h"
#include "tremo_delay.h"

int main(void)
{
    // enable the wdg clk
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_WDG, true);

    uint32_t timeout      = 10000;
    uint32_t wdgclk_freq  = rcc_get_clk_freq(RCC_PCLK0);
    uint32_t reload_value = timeout * (wdgclk_freq / 1000 / 2);

    // start wdg
    wdg_start(reload_value);
    NVIC_EnableIRQ(WDG_IRQn);

    /* Infinite loop */
    while (1) {
        delay_ms(1000);
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
