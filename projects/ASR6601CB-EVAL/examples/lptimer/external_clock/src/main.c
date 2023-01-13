#include <stdio.h>
#include "tremo_regs.h"
#include "tremo_lptimer.h"
#include "tremo_rcc.h"
#include "tremo_gpio.h"


void lptimer_externl_clock()
{
    lptimer_t * LPTIMx = LPTIMER0;
    lptimer_init_t lptimer_init_config;

    lptimer_init_config.count_by_external = false;
    lptimer_init_config.prescaler = LPTIMER_PRESC_1;
    lptimer_init_config.autoreload_preload = false;
    lptimer_init_config.wavpol_inverted = false;
    lptimer_init(LPTIMx, &lptimer_init_config);

    lptimer_config_interrupt(LPTIMx, LPTIMER_IT_CMPM, ENABLE);
    lptimer_config_interrupt(LPTIMx, LPTIMER_IT_ARRM, ENABLE);

    lptimer_cmd(LPTIMx, ENABLE);

    lptimer_set_arr_register(LPTIMx, 0x9);
    lptimer_set_cmp_register(LPTIMx, 0x4);

    lptimer_config_count_mode(LPTIMx, LPTIMER_MODE_CNTSTRT, ENABLE);

    NVIC_EnableIRQ(LPTIMER0_IRQn);
    NVIC_SetPriority(LPTIMER0_IRQn, 2);
}

void lptim_IRQHandler(void)
{
    if( lptimer_get_interrupt_status(LPTIMER0, LPTIMER_IT_CMPM) )
    {
        lptimer_clear_interrupt(LPTIMER0, LPTIMER_IT_CMPM);
    }
    if( lptimer_get_interrupt_status(LPTIMER0, LPTIMER_IT_ARRM) )
    {
        lptimer_clear_interrupt(LPTIMER0, LPTIMER_IT_ARRM);
    }
}

int main(void)
{
    rcc_set_lptimer0_clk_source(RCC_LPTIMER0_CLK_SOURCE_EXTCLK);

    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOB, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOD, true);

    gpio_set_iomux(GPIOD, GPIO_PIN_12, 5); //  IN2:GP60
    gpio_set_iomux(GPIOD, GPIO_PIN_14, 3); //  ETR:GP62
    gpio_set_iomux(GPIOD, GPIO_PIN_10, 2); //  IN1:GP58

    rcc_enable_peripheral_clk(RCC_PERIPHERAL_LPTIMER0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_LPTIMER1, true);

    lptimer_externl_clock();
    
    /* Infinite loop */
    while(1){
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(void *file, uint32_t line)
{
    (void)file;
    (void)line;
    
    while(1){
    }
}
#endif
