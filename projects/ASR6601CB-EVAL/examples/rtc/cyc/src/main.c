#include <stdio.h>
#include "tremo_regs.h"
#include "tremo_rtc.h"
#include "tremo_rcc.h"

void rtc_cyc()
{
    /* NVIC config */
    NVIC_EnableIRQ(RTC_IRQn);
    NVIC_SetPriority(RTC_IRQn, 2);

    rtc_config_cyc_max(32768);
    rtc_config_cyc_wakeup(ENABLE);
    rtc_cyc_cmd(true);
    rtc_config_interrupt(RTC_CYC_IT, ENABLE);
}

void rtc_IRQHandler(void)
{
    uint8_t intr_stat;

    intr_stat = rtc_get_status(RTC_CYC_SR);
    if (intr_stat == true) {
        rtc_config_interrupt(RTC_CYC_IT, DISABLE);
        rtc_set_status(RTC_CYC_SR, false);
        rtc_config_interrupt(RTC_CYC_IT, ENABLE);
    }
}

int main(void)
{
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_AFEC, true);

    // enable the clk
    rcc_enable_oscillator(RCC_OSC_XO32K, true);

    rcc_enable_peripheral_clk(RCC_PERIPHERAL_RTC, true);

    rtc_cyc();

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
