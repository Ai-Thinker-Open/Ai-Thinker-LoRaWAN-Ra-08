#include <stdio.h>
#include "tremo_rcc.h"
#include "tremo_gpio.h"
#include "tremo_delay.h"

#define TEST_GPIOX GPIOA
#define TEST_PIN   GPIO_PIN_8

int main(void)
{
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);
    gpio_init(TEST_GPIOX, TEST_PIN, GPIO_MODE_OUTPUT_PP_HIGH);

    /* Infinite loop */
    while (1) {
        delay_ms(1000);

        gpio_toggle(TEST_GPIOX, TEST_PIN);
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
