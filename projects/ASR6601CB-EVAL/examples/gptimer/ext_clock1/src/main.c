#include <string.h>
#include "tremo_regs.h"
#include "tremo_timer.h"
#include "tremo_rcc.h"
#include "tremo_gpio.h"

void gptimer_ext_clock1(timer_gp_t* TIMERx)
{
    timer_init_t timerx_init;
    timer_slave_config_t slave_config;

    timer_config_interrupt(TIMERx, TIMER_DIER_UIE, ENABLE);
    timer_config_interrupt(TIMERx, TIMER_DIER_CC0IE, ENABLE);
    timer_config_interrupt(TIMERx, TIMER_DIER_CC1IE, ENABLE);
    timer_config_interrupt(TIMERx, TIMER_DIER_CC2IE, ENABLE);
    timer_config_interrupt(TIMERx, TIMER_DIER_CC3IE, ENABLE);
    timer_config_interrupt(TIMERx, TIMER_DIER_TIE, ENABLE);

    slave_config.slave_mode = TIMER_SMS_EXTERNAL1;

    slave_config.input_trigger             = TIMER_TS_TI0FP0;
    slave_config.ic_polarity.cc0p_polarity = TIMER_CC0P_RISING;
    slave_config.ic_filter.ic0f_filter     = TIMER_IC0F_0;
    timer_config_slave(TIMERx, &slave_config);

    TIMERx->CCER &= ~TIMER_CCER_CC0E;

    timerx_init.prescaler          = 0;
    timerx_init.counter_mode       = TIMER_COUNTERMODE_UP;
    timerx_init.period             = 0x8;
    timerx_init.clock_division     = TIMER_CKD_FPCLK_DIV1;
    timerx_init.autoreload_preload = false;
    timer_init(TIMERx, &timerx_init);

    timer_cmd(TIMERx, true);
}

void gptim0_IRQHandler(void)
{
    bool state;

    timer_get_status(TIMER0, TIMER_SR_UIF, &state);

    if (state) {
        timer_clear_status(TIMER0, TIMER_SR_UIF);
    }
}

int main(void)
{
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOB, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOD, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_TIMER0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_TIMER1, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_TIMER2, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_TIMER3, true);

    gpio_set_iomux(GPIOA, GPIO_PIN_10, 7); // ETR
    gpio_set_iomux(GPIOA, GPIO_PIN_3, 6);  // CH3
    gpio_set_iomux(GPIOA, GPIO_PIN_2, 6);  // CH2
    gpio_set_iomux(GPIOA, GPIO_PIN_1, 6);  // CH1
    gpio_set_iomux(GPIOA, GPIO_PIN_0, 6);  // CH0

    gptimer_ext_clock1(TIMER0);

    NVIC_EnableIRQ(TIMER0_IRQn);
    NVIC_SetPriority(TIMER0_IRQn, 2);

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
