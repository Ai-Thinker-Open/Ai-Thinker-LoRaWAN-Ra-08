#include <stdio.h>
#include "tremo_regs.h"
#include "tremo_spi.h"
#include "tremo_gpio.h"
#include "tremo_dma.h"
#include "tremo_dma_handshake.h"

uint8_t data_to_send[10]  = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa };
uint16_t spi0_rx_data[10] = { 0 }, spi1_rx_data[10] = { 0 }, spi2_rx_data[10] = { 0 };
uint8_t rx_idx_spi0 = 0, rx_idx_spi1 = 0, rx_idx_spi2 = 0;

volatile uint8_t is_tx_ssp_dma_done = 0;
volatile uint8_t is_rx_ssp_dma_done = 0;

void tx_ssp_dma_irq_handle(void)
{
    is_tx_ssp_dma_done = 1;
}

void rx_ssp_dma_irq_handle(void)
{
    is_rx_ssp_dma_done = 1;
}

void spi0_dma_tx(void)
{
    ssp_init_t init_struct;
    dma_dev_t dma_dev = { 0 };
    int32_t ret = 0;

    is_tx_ssp_dma_done = is_rx_ssp_dma_done = 0;

    ssp_init_struct(&init_struct);
    init_struct.ssp_dma_tx_en = ENABLE;

    ssp_init(SSP0, &init_struct);

    ssp_cmd(SSP0, ENABLE);

    dma_dev.dma_num    = 0;
    dma_dev.ch         = 3;
    dma_dev.mode       = M2P_MODE;
    dma_dev.src        = (uint32_t)data_to_send;
    dma_dev.dest       = (uint32_t) & (SSP0->DR);
    dma_dev.priv       = (dma_callback_func)tx_ssp_dma_irq_handle;
    dma_dev.data_width = 0;
    dma_dev.block_size = 10;
    dma_dev.handshake  = DMA_HANDSHAKE_SSP_0_TX;
    dma_dev.src_msize  = 1;
    dma_dev.dest_msize = 1;

    dma_init(&dma_dev);
    dma_ch_enable(dma_dev.dma_num, dma_dev.ch);

    if (ret != 0) {
        return;
    }

    while (is_tx_ssp_dma_done == 0) {
        ;
    }

    dma_finalize(&dma_dev);
}

int main(void)
{
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOB, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOD, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SYSCFG, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SSP0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SSP1, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SSP2, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_DMA0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_DMA1, true);

    gpio_set_iomux(GPIOA, GPIO_PIN_0, 4); // CLK:GP0
    gpio_set_iomux(GPIOA, GPIO_PIN_1, 4); // NSS:GP1
    gpio_set_iomux(GPIOA, GPIO_PIN_2, 4); // TX:GP2
    gpio_set_iomux(GPIOA, GPIO_PIN_3, 4); // RX:GP3

    spi0_dma_tx();

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
