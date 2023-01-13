#include <stdio.h>
#include "tremo_regs.h"
#include "tremo_bstimer.h"
#include "tremo_rcc.h"

void bstimer_onepulse()
{
    bstimer_init_t bstimer_init_config;

    bstimer_init_config.bstimer_mms        = BSTIMER_MMS_ENABLE;
    bstimer_init_config.period             = 2399;  //time period is ((1 / 2.4k) * (2399 + 1))
    bstimer_init_config.prescaler          = 9999;  //sysclock defaults to 24M, is divided by (prescaler + 1) to 2.4k  
    bstimer_init_config.autoreload_preload = false;
    bstimer_init(BSTIMER0, &bstimer_init_config);

    bstimer_config_one_pulse(BSTIMER0, ENABLE);

    bstimer_config_overflow_update(BSTIMER0, ENABLE);
    bstimer_generate_event(BSTIMER0, BSTIMER_EGR_UG, ENABLE); //in order to make prescaler work in the first time period 

    bstimer_config_interrupt(BSTIMER0, ENABLE);

    bstimer_cmd(BSTIMER0, true);

    NVIC_EnableIRQ(BSTIMER0_IRQn);
    NVIC_SetPriority(BSTIMER0_IRQn, 2);
}

void btim0_IRQHandler(void)
{
    if (bstimer_get_status(BSTIMER0, BSTIMER_SR_UIF)) {
        //UIF flag is active
    } else {
    }
}

int main(void)
{
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_BSTIMER0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_BSTIMER1, true);

    bstimer_onepulse();

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
