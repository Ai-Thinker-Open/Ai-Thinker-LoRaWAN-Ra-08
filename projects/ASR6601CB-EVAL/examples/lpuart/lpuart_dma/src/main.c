#include <stdio.h>
#include "tremo_regs.h"
#include "tremo_lpuart.h"
#include "tremo_rcc.h"
#include "tremo_dma.h"
#include "tremo_dma_handshake.h"
#include "tremo_gpio.h"

volatile uint8_t is_lpuart_tx_dma_done = 0;
volatile uint8_t is_lpuart_rx_dma_done = 0;

uint8_t rx_data[11] = { 0 };

void tx_lpuart_dma_irq_handle(void)
{
    is_lpuart_tx_dma_done = 1;
}

void rx_lpuart_dma_irq_handle(void)
{
    is_lpuart_rx_dma_done = 1;
}

void lpuart_dma()
{
    lpuart_init_t lpuart_init_cofig;
    dma_dev_t dma_dev = { 0 };

    gpio_set_iomux(GPIOC, GPIO_PIN_15, 2); // TX:GP47
    gpio_set_iomux(GPIOD, GPIO_PIN_12, 2); // RX:GP60

    lpuart_init_cofig.baudrate         = 9600;
    lpuart_init_cofig.data_width       = LPUART_DATA_8BIT;
    lpuart_init_cofig.parity           = LPUART_PARITY_NONE;
    lpuart_init_cofig.stop_bits        = LPUART_STOP_1BIT;
    lpuart_init_cofig.low_level_wakeup = true;
    lpuart_init_cofig.start_wakeup     = false;
    lpuart_init_cofig.rx_done_wakeup   = false;
    lpuart_init(LPUART, &lpuart_init_cofig);

    lpuart_config_rx(LPUART, true);

    lpuart_config_dma(LPUART, LPUART_CR1_RX_DMA, true);

    dma_dev.src        = (uint32_t) & (LPUART->DATA);
    dma_dev.dest       = (uint32_t)(rx_data);
    dma_dev.data_width = 0;
    dma_dev.src_msize  = 0;
    dma_dev.dest_msize = 0;
    dma_dev.ch         = 1;
    dma_dev.block_size = 5;
    dma_dev.handshake  = DMA_HANDSHAKE_LPUART_RX;
    dma_dev.dma_num    = 1;
    dma_dev.mode       = P2M_MODE;
    dma_dev.priv       = rx_lpuart_dma_irq_handle;

    dma_init(&dma_dev);
    dma_ch_enable(dma_dev.dma_num, dma_dev.ch);

    while (is_lpuart_rx_dma_done == 0) {
        ;
    }

    dma_finalize(&dma_dev);
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
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_LPUART, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SYSCFG, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_DMA0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_DMA1, true);

    lpuart_dma();

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
