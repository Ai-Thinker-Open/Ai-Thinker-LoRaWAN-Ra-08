#include <stdio.h>
#include "tremo_rcc.h"
#include "tremo_iwdg.h"
#include "tremo_delay.h"

int main(void)
{
    rcc_enable_oscillator(RCC_OSC_XO32K, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_IWDG, true);

    iwdg_init(true);

    iwdg_set_prescaler(IWDG_PRESCALER_256);
    iwdg_set_reload(0x3FF); // 8s
    iwdg_start();

    /* Infinite loop */
    while (1) {
        delay_ms(4000);

        // feed the watch dog
        iwdg_reload();
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
