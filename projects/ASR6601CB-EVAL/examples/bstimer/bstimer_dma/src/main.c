#include <stdio.h>
#include "tremo_bstimer.h"
#include "tremo_rcc.h"
#include "tremo_dma.h"
#include "tremo_dma_handshake.h"

volatile uint8_t is_write_bstimer_dma_done = 0;
volatile uint8_t is_read_bstimer_dma_done  = 0;

uint16_t bstimer_data = 0;

void write_bstimer_dma_irq_handle(void)
{
    is_write_bstimer_dma_done = 1;
}

void read_bstimer_dma_irq_handle(void)
{
    is_read_bstimer_dma_done = 1;
}

void bstimer_dma()
{
    bstimer_init_t bstimer_init_config;
    dma_dev_t dma_dev = { 0 };

    bstimer_init_config.bstimer_mms        = BSTIMER_MMS_ENABLE;
    bstimer_init_config.period             = 0xbb;
    bstimer_init_config.prescaler          = 0;
    bstimer_init_config.autoreload_preload = false;
    bstimer_init(BSTIMER0, &bstimer_init_config);

    bstimer_config_overflow_update(BSTIMER0, DISABLE);
    bstimer_config_update_disable(BSTIMER0, DISABLE);

    bstimer_config_dma(BSTIMER0, ENABLE);

    bstimer_data = 0xaa;

    dma_dev.dma_num = 0;

    dma_dev.ch         = 0;
    dma_dev.mode       = M2P_MODE;
    dma_dev.src        = (uint32_t)(&bstimer_data);
    dma_dev.dest       = (uint32_t)(&(BSTIMER0->ARR));
    dma_dev.priv       = write_bstimer_dma_irq_handle;
    dma_dev.src_msize  = 0;
    dma_dev.dest_msize = 0;
    dma_dev.data_width = 1;
    dma_dev.block_size = 1;
    dma_dev.handshake  = DMA_HANDSHAKE_BSTIMER_0_UP;

    dma_init(&dma_dev);
    dma_ch_enable(dma_dev.dma_num, dma_dev.ch);

    bstimer_cmd(BSTIMER0, true);

    while (is_write_bstimer_dma_done == 0) {
        ;
    }

    dma_finalize(&dma_dev);
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
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SYSCFG, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_DMA0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_DMA1, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_BSTIMER0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_BSTIMER1, true);

    bstimer_dma();

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
