#include <stdio.h>
#include "tremo_regs.h"
#include "tremo_i2s.h"
#include "tremo_rcc.h"
#include "tremo_gpio.h"

void i2s_master()
{
    uint32_t ldata[] = { 0x11111111, 0x22222222, 0x33333333, 0x44444444, 0x55555555 };
    uint32_t rdata[] = { 0xaaaaaaaa, 0xbbbbbbbb, 0xcccccccc, 0xdddddddd, 0xeeeeeeee };
    i2s_init_t i2s_struct;
    uint8_t freq_devision;

    NVIC_EnableIRQ(I2S_IRQn);
    NVIC_SetPriority(I2S_IRQn, 2);

    i2s_init_struct(&i2s_struct);
    i2s_init(I2S, &i2s_struct);
    if (i2s_struct.i2s_role == I2S_ROLE_MASTER) {
        freq_devision = i2s_calculate_devision(i2s_struct.i2s_word_size);
        SYSCFG->CR10 |= 1 << I2S_MASTER_ENABLE_POS;       // enable master
        SYSCFG->CR10 |= freq_devision << I2S_WS_FREQ_POS; // config ws frequence
    }

    i2s_tx_block_cmd(I2S, ENABLE);

    if (i2s_struct.i2s_role == I2S_ROLE_MASTER) {
        SYSCFG->CR10 |= 1 << I2S_WS_ENABLE_POS; // enable ws output
    }

    i2s_send_data(I2S, (uint16_t *)ldata, (uint16_t *)rdata, 5);
}

void i2s_IRQHandler(void)
{
    uint32_t data_l = 0;
    uint32_t data_r = 0;

    if (i2s_get_interrupt_status(I2S, I2S_INTERRUPT_RXDA)) // rx data available
    {
        data_l = i2s_receive_data(I2S, I2S_LEFT_CHANNEL);
        data_r = i2s_receive_data(I2S, I2S_RIGHT_CHANNEL);
    }
    if (i2s_get_interrupt_status(I2S, I2S_INTERRUPT_TXFE)) // tx empty
    {
        data_r = 0;
    }
}

int main(void)
{
    uint8_t freq_devision;

    freq_devision = i2s_calculate_devision(I2S_WORDSIZE_32bit);
    rcc_set_i2s_clk_source(RCC_I2S_CLK_SOURCE_PCLK0);
    rcc_set_i2s_mclk_div(2);
    rcc_set_i2s_sclk_div((RCC_FREQ_24M / 2) / I2S_SAMPLE_RATE_44P1K / freq_devision);

    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOB, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOD, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SYSCFG, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_I2S, true);

    gpio_set_iomux(GPIOA, GPIO_PIN_1, 5); // MCLK:GP1
    gpio_set_iomux(GPIOB, GPIO_PIN_1, 6); // SCLK_OUT:GP17
    gpio_set_iomux(GPIOA, GPIO_PIN_3, 2); // DO:GP3
    gpio_set_iomux(GPIOA, GPIO_PIN_4, 6); // WS_OUT:GP4

    i2s_master();

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
