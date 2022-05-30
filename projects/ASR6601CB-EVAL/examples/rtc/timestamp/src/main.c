#include <stdio.h>
#include "tremo_regs.h"
#include "tremo_rtc.h"
#include "tremo_rcc.h"

void rtc_timestamp()
{
    rtc_calendar_t time = { 2019, 6, 12, 31, 23, 59, 55, 0 };

    rtc_set_calendar(&time);
    rtc_calendar_cmd(true);

    rtc_get_calendar(&time);
}

void rtc_IRQHandler(void)
{
    uint8_t intr_stat;

    intr_stat = rtc_get_status(RTC_ALARM0_SR);
    if (intr_stat == true) {
        rtc_config_interrupt(RTC_ALARM0_IT, DISABLE);
        rtc_set_status(RTC_ALARM0_SR, false);
        rtc_config_interrupt(RTC_ALARM0_IT, ENABLE);
    }
}

int main(void)
{
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_AFEC, true);

    // enable the clk
    rcc_enable_oscillator(RCC_OSC_XO32K, true);

    rcc_enable_peripheral_clk(RCC_PERIPHERAL_RTC, true);

    rtc_timestamp();

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
