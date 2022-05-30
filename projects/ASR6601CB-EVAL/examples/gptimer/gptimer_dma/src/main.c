#include "tremo_timer.h"
#include "tremo_rcc.h"
#include "tremo_gpio.h"
#include "tremo_dma.h"
#include "tremo_dma_handshake.h"

volatile uint8_t is_write_timer_dma_done = 0;
volatile uint8_t is_read_timer_dma_done  = 0;
uint16_t dma_mem[11]                     = { 0 };

void write_timer_dma_irq_handle(void)
{
    is_write_timer_dma_done = 1;
}

void read_timer_dma_irq_handle(void)
{
    is_read_timer_dma_done = 1;
}

void timers_dma_upd(timer_gp_t* TIMERx)
{
    timer_init_t timerx_init;
    dma_dev_t dma_dev = { 0 };

    timer_config_interrupt(TIMERx, TIMER_DIER_CC0IE, ENABLE);
    timer_config_interrupt(TIMERx, TIMER_DIER_CC1IE, ENABLE);
    timer_config_interrupt(TIMERx, TIMER_DIER_CC2IE, ENABLE);
    timer_config_interrupt(TIMERx, TIMER_DIER_CC3IE, ENABLE);
    timer_config_interrupt(TIMERx, TIMER_DIER_TIE, ENABLE);

    is_write_timer_dma_done = is_read_timer_dma_done = 0;

    timerx_init.prescaler          = 0;
    timerx_init.counter_mode       = TIMER_COUNTERMODE_UP;
    timerx_init.period             = 0xbb;
    timerx_init.clock_division     = TIMER_CKD_FPCLK_DIV1;
    timerx_init.autoreload_preload = true;
    timer_init(TIMERx, &timerx_init);

    timer_config_dma(TIMERx, TIMER_DIER_UDE, ENABLE);

    dma_mem[0] = 0xaa;
    dma_mem[1] = 0xaa;
    dma_mem[2] = 0xaa;
    dma_mem[3] = 0xaa;

    dma_dev.dma_num    = 0;
    dma_dev.ch         = 3;
    dma_dev.mode       = M2P_MODE;
    dma_dev.src        = (uint32_t)(dma_mem);
    dma_dev.dest       = (uint32_t) & (TIMERx->DMAR);
    dma_dev.priv       = (dma_callback_func)write_timer_dma_irq_handle;
    dma_dev.src_msize  = 1;
    dma_dev.dest_msize = 1;
    dma_dev.data_width = 1;
    dma_dev.block_size = (TIMER_DBL_4 >> TIMER_DCR_DBL_POSITION) + 1;
    dma_dev.handshake  = DMA_HANDSHAKE_TIMER_0_UP;

    timer_set_dma_rw_len(TIMERx, TIMER_DBL_4);
    timer_set_dma_base_addr(TIMERx, TIMER_DBA_CCR0);

    dma_init(&dma_dev);
    dma_ch_enable(dma_dev.dma_num, dma_dev.ch);

    timer_cmd(TIMERx, true);

    while (is_write_timer_dma_done == 0) {
        ;
    }

    dma_finalize(&dma_dev);
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
    timer_deinit(TIMER0);
    dma_deinit(0);

    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOB, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOD, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SYSCFG, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_DMA0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_DMA1, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_TIMER0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_TIMER1, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_TIMER2, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_TIMER3, true);

    gpio_set_iomux(GPIOA, GPIO_PIN_10, 7); // ETR
    gpio_set_iomux(GPIOA, GPIO_PIN_3, 6);  // CH3
    gpio_set_iomux(GPIOA, GPIO_PIN_2, 6);  // CH2
    gpio_set_iomux(GPIOA, GPIO_PIN_1, 6);  // CH1
    gpio_set_iomux(GPIOA, GPIO_PIN_0, 6);  // CH0

    timers_dma_upd(TIMER0);

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
