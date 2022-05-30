#include <stdio.h>
#include "tremo_regs.h"
#include "tremo_i2s.h"
#include "tremo_rcc.h"
#include "tremo_gpio.h"


uint32_t g_data_l[5] = {0};
uint32_t g_data_r[5] = {0};
uint32_t g_index = 0;

void i2s_slave()
{
    i2s_init_t i2s_struct;
    uint8_t freq_devision;

    NVIC_EnableIRQ(I2S_IRQn);
    NVIC_SetPriority(I2S_IRQn, 2);

    i2s_init_struct(&i2s_struct);
    i2s_struct.i2s_role = I2S_ROLE_SLAVE;
    i2s_init(I2S, &i2s_struct);
    if (i2s_struct.i2s_role == I2S_ROLE_MASTER) {
        freq_devision = i2s_calculate_devision(i2s_struct.i2s_word_size);
        SYSCFG->CR10 |= 1 << I2S_MASTER_ENABLE_POS;       // enable master
        SYSCFG->CR10 |= freq_devision << I2S_WS_FREQ_POS; // config ws frequence
    }

    i2s_rx_block_cmd(I2S, ENABLE);

    if (i2s_struct.i2s_role == I2S_ROLE_MASTER) {
        SYSCFG->CR10 |= 1 << I2S_WS_ENABLE_POS; // enable ws output
    }

    i2s_config_interrupt(I2S, I2S_INTERRUPT_RXDA, ENABLE);
}

void i2s_IRQHandler(void)
{
    // rx data available
    if (i2s_get_interrupt_status(I2S, I2S_INTERRUPT_RXDA)) {
        g_data_l[g_index] = i2s_receive_data(I2S, I2S_LEFT_CHANNEL);
        g_data_r[g_index] = i2s_receive_data(I2S, I2S_RIGHT_CHANNEL);
        if ((g_data_l[g_index] != 0) || (g_data_r[g_index] != 0)) {
            if (g_index < 4) {
                g_index++;
            } else {
                g_index = 0;
            }
        }
    }

    // tx empty
    if (i2s_get_interrupt_status(I2S, I2S_INTERRUPT_TXFE)) {
    }
}

int main(void)
{
    rcc_set_i2s_clk_source(RCC_I2S_CLK_SOURCE_EXT_CLK);

    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOB, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOD, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SYSCFG, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_I2S, true);

    gpio_set_iomux(GPIOA, GPIO_PIN_2, 2); // DI:GP2
    gpio_set_iomux(GPIOB, GPIO_PIN_1, 5); // SCLK_IN:GP17
    gpio_set_iomux(GPIOA, GPIO_PIN_4, 5); // WS_IN:GP4

    i2s_slave();

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
