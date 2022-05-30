#include <stdio.h>
#include "tremo_regs.h"
#include "tremo_lptimer.h"
#include "tremo_rcc.h"
#include "tremo_pwr.h"
#include "tremo_gpio.h"

void lptimer_wakeup()
{
    lptimer_t* LPTIMx = LPTIMER0;
    lptimer_init_t lptimer_init_config;

    gpio_set_iomux(GPIOD, GPIO_PIN_12, 5); //  IN2:GP60
    gpio_set_iomux(GPIOD, GPIO_PIN_14, 3); //  ETR:GP62
    gpio_set_iomux(GPIOD, GPIO_PIN_10, 2); //  IN1:GP58

    rcc_enable_peripheral_clk(RCC_PERIPHERAL_LPTIMER0, false);
    rcc_rst_peripheral(RCC_PERIPHERAL_LPTIMER0, true);
    rcc_rst_peripheral(RCC_PERIPHERAL_LPTIMER0, false);
    rcc_set_lptimer0_clk_source(RCC_LPTIMER0_CLK_SOURCE_XO32K);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_LPTIMER0, true);

    lptimer_init_config.sel_external_clock = false;
    lptimer_init_config.count_by_external  = false;
    lptimer_init_config.prescaler          = LPTIMER_PRESC_1;
    lptimer_init_config.autoreload_preload = false;
    lptimer_init_config.wavpol_inverted    = false;
    lptimer_init(LPTIMx, &lptimer_init_config);

    lptimer_config_interrupt(LPTIMx, LPTIMER_IT_CMPM, ENABLE);
    lptimer_config_interrupt(LPTIMx, LPTIMER_IT_ARRM, ENABLE);

    lptimer_config_wakeup(LPTIMx, LPTIMER_CFGR_CMPM_WKUP, ENABLE);

    lptimer_cmd(LPTIMx, ENABLE);

    lptimer_set_arr_register(LPTIMx, 0xFFFF);
    lptimer_set_cmp_register(LPTIMx, 0xF000);

    lptimer_config_count_mode(LPTIMx, LPTIMER_MODE_CNTSTRT, ENABLE);

    NVIC_EnableIRQ(LPTIMER0_IRQn);
    NVIC_SetPriority(LPTIMER0_IRQn, 2);
}

void lptim_IRQHandler(void)
{
    if( lptimer_get_interrupt_status(LPTIMER0, LPTIMER_IT_CMPM) )
    {
        lptimer_clear_interrupt(LPTIMER0, LPTIMER_IT_CMPM);
        while(lptimer_get_clear_status_flag(LPTIMER0, LPTIMER_CSR_CMPM) == false);
    }
    if( lptimer_get_interrupt_status(LPTIMER0, LPTIMER_IT_ARRM) )
    {
        lptimer_clear_interrupt(LPTIMER0, LPTIMER_IT_ARRM);
        while(lptimer_get_clear_status_flag(LPTIMER0, LPTIMER_CSR_ARRM) == false);
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
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_LPTIMER0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_LPTIMER1, true);

    lptimer_wakeup();

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
