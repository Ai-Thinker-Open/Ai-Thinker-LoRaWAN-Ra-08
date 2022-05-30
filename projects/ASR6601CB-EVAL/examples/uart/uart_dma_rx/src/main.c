#include <stdio.h>
#include <stdarg.h>
#include "tremo_uart.h"
#include "tremo_dma.h"
#include "tremo_dma_handshake.h"
#include "tremo_gpio.h"
#include "tremo_rcc.h"

static int volatile is_rx_uart_dma_done = 0;
static uint8_t buf[12]         = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B };

void rx_uart_dma_irq_handle(void)
{
    is_rx_uart_dma_done = 1;
}

int main(void)
{
    uart_t* uartx = UART0;
    dma_dev_t dma_dev;

    rcc_enable_peripheral_clk(RCC_PERIPHERAL_UART0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOB, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SYSCFG, true);
    
    dma_deinit(0);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_DMA0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_DMA1, true);

    gpio_set_iomux(GPIOB, GPIO_PIN_0, 1);
    gpio_set_iomux(GPIOB, GPIO_PIN_1, 1);

    uart_config_t uart_init_struct;
    uart_config_init(&uart_init_struct);
    uart_init_struct.fifo_mode = ENABLE;
    uart_set_rx_fifo_threshold(uartx, UART_IFLS_RX_1_4);

    uart_init(uartx, &uart_init_struct);
    uart_cmd(uartx, ENABLE);

    dma_dev.dma_num    = 0;
    dma_dev.ch         = 0;
    dma_dev.mode       = P2M_MODE;
    dma_dev.src        = (uint32_t) & (uartx->DR);
    dma_dev.dest       = (uint32_t)(buf);
    dma_dev.priv       = rx_uart_dma_irq_handle;
    dma_dev.data_width = 0;
    dma_dev.block_size = 12;
    dma_dev.src_msize  = 1;
    dma_dev.dest_msize = 1;
    dma_dev.handshake  = DMA_HANDSHAKE_UART_0_RX;

    dma_init(&dma_dev);
    dma_ch_enable(dma_dev.dma_num, 0);
    uart_dma_config(uartx, UART_DMA_REQ_RX, true);

    while (is_rx_uart_dma_done == 0) {
        ;
    }

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
