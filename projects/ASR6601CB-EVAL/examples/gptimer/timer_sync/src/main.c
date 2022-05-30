#include "tremo_regs.h"
#include "tremo_timer.h"
#include "tremo_rcc.h"
#include "tremo_gpio.h"

void gptimer_timer_sync(timer_gp_t* TIMERx, timer_gp_t* TIMERy, timer_ts_t ITRx)
{
    timer_init_t timerx_init;
    timer_init_t timery_init;
    timer_slave_config_t slave_config;

    timer_config_interrupt(TIMERy, TIMER_DIER_UIE, ENABLE);
    timer_config_interrupt(TIMERy, TIMER_DIER_CC0IE, ENABLE);
    timer_config_interrupt(TIMERy, TIMER_DIER_CC1IE, ENABLE);
    timer_config_interrupt(TIMERy, TIMER_DIER_CC2IE, ENABLE);
    timer_config_interrupt(TIMERy, TIMER_DIER_CC3IE, ENABLE);
    timer_config_interrupt(TIMERy, TIMER_DIER_TIE, ENABLE);

    slave_config.slave_mode    = TIMER_SMS_TRIGGER;
    slave_config.input_trigger = ITRx;
    timer_config_slave(TIMERy, &slave_config);

    timery_init.prescaler          = 0;
    timery_init.counter_mode       = TIMER_COUNTERMODE_UP;
    timery_init.period             = 0xbb;
    timery_init.clock_division     = TIMER_CKD_FPCLK_DIV1;
    timery_init.autoreload_preload = false;
    timer_init(TIMERy, &timery_init);

    timerx_init.prescaler          = 0;
    timerx_init.counter_mode       = TIMER_COUNTERMODE_UP;
    timerx_init.period             = 0xbb;
    timerx_init.clock_division     = TIMER_CKD_FPCLK_DIV1;
    timerx_init.autoreload_preload = false;
    timer_config_master_mode(TIMERx, TIMER_TRGO_ENABLE);
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

void gptim1_IRQHandler(void)
{
    bool state;

    timer_get_status(TIMER1, TIMER_SR_UIF, &state);

    if (state) {
        timer_clear_status(TIMER1, TIMER_SR_UIF);
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

    gptimer_timer_sync(TIMER0, TIMER1, TIMER_TS_ITR0);

    NVIC_EnableIRQ(TIMER1_IRQn);
    NVIC_SetPriority(TIMER1_IRQn, 2);

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
