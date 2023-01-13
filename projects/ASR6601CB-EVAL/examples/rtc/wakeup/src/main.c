#include <stdio.h>
#include "tremo_regs.h"
#include "tremo_rtc.h"
#include "tremo_rcc.h"
#include "tremo_gpio.h"

void rtc_wakeup()
{
    gpio_set_iomux(GPIOD, GPIO_PIN_14, 5); // RTC_WAKEUP1:GP62

    /* NVIC config */
    NVIC_EnableIRQ(RTC_IRQn);
    NVIC_SetPriority(RTC_IRQn, 2);

    rtc_config_wakeup_io_filter(1, RTC_FILTER_7);
    rtc_config_wakeup_io_high_level(1, ENABLE);
    rtc_config_wakeup_io_wakeup(1, ENABLE);
    rtc_wakeup_io_cmd(1, true);
    rtc_config_interrupt(RTC_WAKEUP1_IT, ENABLE);
}

void rtc_IRQHandler(void)
{
    uint8_t intr_stat;

    intr_stat = rtc_get_status(RTC_WAKEUP1_SR);
    if (intr_stat == true) {
        rtc_config_interrupt(RTC_WAKEUP1_IT, DISABLE);
        rtc_set_status(RTC_WAKEUP1_SR, false);
        rtc_config_interrupt(RTC_WAKEUP1_IT, ENABLE);
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
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_RTC, true);

    rtc_wakeup();

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
