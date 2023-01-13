#include <stdio.h>
#include "tremo_regs.h"
#include "tremo_dma.h"

volatile uint8_t is_tx_mem_dma_done = 0;
volatile uint8_t is_rx_mem_dma_done = 0;

uint8_t dma_mem_to_send[8]  = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };
uint8_t dma_data_to_recv[8] = { 0 };

void tx_mem_dma_irq_handle(void)
{
    is_tx_mem_dma_done = 1;
}

void rx_mem_dma_irq_handle(void)
{
    is_rx_mem_dma_done = 1;
}

int32_t trans_mem_data_by_dma(
    void* src, void* dst, uint16_t tx_size, uint16_t rx_size, uint16_t dma_num, uint16_t dma_chnl)
{
    dma_dev_t dma_dev;

    is_tx_mem_dma_done = is_rx_mem_dma_done = 0;

    dma_dev.dma_num    = 0;
    dma_dev.ch         = dma_chnl;
    dma_dev.mode       = M2M_MODE;
    dma_dev.src        = (uint32_t)(src);
    dma_dev.dest       = (uint32_t)(dst);
    dma_dev.priv       = (dma_callback_func)rx_mem_dma_irq_handle;
    dma_dev.data_width = 0;
    dma_dev.block_size = tx_size;
    dma_dev.src_msize  = 0;
    dma_dev.dest_msize = 0;

    dma_init(&dma_dev);
    dma_ch_enable(dma_dev.dma_num, dma_chnl);

    while (is_rx_mem_dma_done == 0) {
        ;
    }
    is_rx_mem_dma_done = 0;

    dma_finalize(&dma_dev);

    return 1;
}

int main(void)
{
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_DMA0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_DMA1, true);

    (void)trans_mem_data_by_dma(dma_mem_to_send, dma_data_to_recv, 8, 8, 0, 0);

    /* Infinite loop */
    while (1) { }

    return 0;
}

#ifdef USE_FULL_ASSERT
void assert_failed(void* file, uint32_t line)
{
    (void)file;
    (void)line;

    while (1) { }
}
#endif
