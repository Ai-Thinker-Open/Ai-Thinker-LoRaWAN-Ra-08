#include <stdio.h>
#include "tremo_rcc.h"
#include "tremo_dma.h"
#include "tremo_dma_handshake.h"
#include "tremo_gpio.h"
#include "tremo_i2c.h"

static volatile int is_tx_i2c_master_dma_done = 0;
static uint32_t buf[11]              = { 0x0, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };

void tx_i2c_master_dma_irq_handle(void)
{
    is_tx_i2c_master_dma_done = 1;
}

int main(void)
{
    dma_dev_t dma_dev;
    uint32_t slave_addr = 0x76;
    i2c_config_t config;

    // enable the clk
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_I2C0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SYSCFG, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_DMA0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_DMA1, true);

    // set iomux
    gpio_set_iomux(GPIOA, GPIO_PIN_14, 3);
    gpio_set_iomux(GPIOA, GPIO_PIN_15, 3);

    buf[0] = (1 << 11) | (0 << 10) | (0 << 9) | (1 << 8) | (slave_addr << 1) | I2C_WRITE;
    for (int i = 1; i < 10; i++) {
        buf[i] |= (1 << 11) | (0 << 10) | (0 << 9) | (0 << 8);
    }
    buf[10] |= (1 << 11) | (0 << 10) | (1 << 9) | (0 << 8);

    // init
    i2c_config_init(&config);
    config.fifo_mode_en = true;
    i2c_init(I2C0, &config);
    i2c_cmd(I2C0, true);

    dma_dev.dma_num    = 0;
    dma_dev.ch         = 0;
    dma_dev.mode       = M2P_MODE;
    dma_dev.src        = (uint32_t)(buf);
    dma_dev.dest       = (uint32_t) & (I2C0->WFIFO);
    dma_dev.priv       = tx_i2c_master_dma_irq_handle;
    dma_dev.data_width = 2;
    dma_dev.block_size = 11;
    dma_dev.src_msize  = 0;
    dma_dev.dest_msize = 0;
    dma_dev.handshake  = DMA_HANDSHAKE_I2C_0_TX;

    dma_init(&dma_dev);
    dma_ch_enable(dma_dev.dma_num, 0);

    i2c_dma_cmd(I2C0, true);

    while (is_tx_i2c_master_dma_done == 0) {
        ;
    }

    while (i2c_get_flag_status(I2C0, I2C_FLAG_TRANS_DONE) != SET)
        ;
    i2c_clear_flag_status(I2C0, I2C_FLAG_TRANS_DONE);

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
