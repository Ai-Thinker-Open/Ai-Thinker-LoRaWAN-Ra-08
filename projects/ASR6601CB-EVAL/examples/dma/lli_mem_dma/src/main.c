#include <stdio.h>
#include "tremo_regs.h"
#include "tremo_dma.h"

volatile uint8_t is_tx_mem_dma_done = 0;
volatile uint8_t is_rx_mem_dma_done = 0;

uint8_t dma_mem_to_send[11]  = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x0, 0x66, 0x77, 0x88, 0x99, 0xaa };
uint8_t dma_data_to_recv[11] = { 0 };

dma_lli_t dma_lli[2] = { { 0 } };

void tx_mem_dma_irq_handle(void)
{
    is_tx_mem_dma_done = 1;
}

void rx_mem_dma_irq_handle(void)
{
    is_rx_mem_dma_done = 1;
}

int32_t trans_mem_data_by_dma(void* src, void* dst, uint16_t dma_num, uint16_t dma_chnl)
{
    dma_dev_t dma_dev;
    dma_lli_block_config_t dma_lli_block[2];
    dma_lli_mode_t lli_mode;

    is_tx_mem_dma_done = is_rx_mem_dma_done = 0;

    dma_dev.dma_num    = dma_num;
    dma_dev.ch         = dma_chnl;
    dma_dev.mode       = M2M_MODE;
    dma_dev.src        = (uint32_t)(src);
    dma_dev.dest       = (uint32_t)(dst);
    dma_dev.priv       = (dma_callback_func)rx_mem_dma_irq_handle;
    dma_dev.data_width = 0;
    dma_dev.block_size = 5;
    dma_dev.src_msize  = 0;
    dma_dev.dest_msize = 0;

    dma_lli_block[0].src        = (uint32_t)(src);
    dma_lli_block[0].dest       = (uint32_t)(dst);
    dma_lli_block[0].data_width = 0;
    dma_lli_block[0].src_msize  = 0;
    dma_lli_block[0].dest_msize = 0;
    dma_lli_block[0].block_size = 5;

    dma_lli_block[1].src        = (uint32_t)((uint32_t)src + 6);
    dma_lli_block[1].dest       = (uint32_t)((uint32_t)dst + 6);
    dma_lli_block[1].data_width = 0;
    dma_lli_block[1].src_msize  = 0;
    dma_lli_block[1].dest_msize = 0;
    dma_lli_block[1].block_size = 4;

    lli_mode.block_num       = 2;
    lli_mode.src_lli_enable  = true;
    lli_mode.dest_lli_enable = true;

    dma_lli_init(&dma_dev, dma_lli, dma_lli_block, &lli_mode);
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

    (void)trans_mem_data_by_dma(dma_mem_to_send, dma_data_to_recv, 0, 0);

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
