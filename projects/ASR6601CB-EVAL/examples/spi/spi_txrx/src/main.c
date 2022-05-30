#include <stdio.h>
#include "tremo_regs.h"
#include "tremo_rcc.h"
#include "tremo_spi.h"
#include "tremo_gpio.h"

uint8_t data_to_send[10] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa };
uint8_t spi0_rx_data[10] = { 0 }, spi1_rx_data[10] = { 0 }, spi2_rx_data[10] = { 0 };
uint8_t rx_idx_spi0 = 0, rx_idx_spi1 = 0, rx_idx_spi2 = 0;

void spi0_tx_rx(void)
{
    int i;

    ssp_init_t init_struct;

    ssp_init_struct(&init_struct);

    ssp_init(SSP0, &init_struct);

    ssp_config_interrupt(SSP0, SSP_INTERRUPT_RX_FIFO_TRIGGER | SSP_INTERRUPT_RX_TIMEOUT | SSP_INTERRUPT_RX_FIFO_OVERRUN, ENABLE);

    /* NVIC config */
    NVIC_EnableIRQ(SSP0_IRQn);
    NVIC_SetPriority(SSP0_IRQn, 2);
    ssp_cmd(SSP0, ENABLE);

    for (i = 0; i < 10; i++) {
        ssp_send_data(SSP0, data_to_send + i, 1);
    }
}

/* spi interrupt handler */
void ssp0_IRQHandler(void)
{
    if (ssp_get_interrupt_status(SSP0, SSP_INTERRUPT_TX_FIFO_TRIGGER)) {
        ssp_config_interrupt(SSP0, SSP_INTERRUPT_TX_FIFO_TRIGGER, DISABLE);
        ssp_clear_interrupt(SSP0, SSP_INTERRUPT_TX_FIFO_TRIGGER);
        ssp_config_interrupt(SSP0, SSP_INTERRUPT_TX_FIFO_TRIGGER, ENABLE);
    }
    if (ssp_get_interrupt_status(SSP0, SSP_INTERRUPT_RX_FIFO_TRIGGER)) {
        ssp_config_interrupt(SSP0, SSP_INTERRUPT_RX_FIFO_TRIGGER, DISABLE);
        while (ssp_get_flag_status(SSP0, SSP_FLAG_RX_FIFO_NOT_EMPTY)) {
            spi0_rx_data[rx_idx_spi0++] = SSP0->DR;
        }
        ssp_clear_interrupt(SSP0, SSP_INTERRUPT_RX_FIFO_TRIGGER);
        ssp_config_interrupt(SSP0, SSP_INTERRUPT_RX_FIFO_TRIGGER, ENABLE);
    }
    if (ssp_get_interrupt_status(SSP0, SSP_INTERRUPT_RX_FIFO_OVERRUN)) {
        ssp_config_interrupt(SSP0, SSP_INTERRUPT_RX_FIFO_OVERRUN, DISABLE);
        ssp_clear_interrupt(SSP0, SSP_INTERRUPT_RX_FIFO_OVERRUN);
        ssp_config_interrupt(SSP0, SSP_INTERRUPT_RX_FIFO_OVERRUN, ENABLE);
    }
    if (ssp_get_interrupt_status(SSP0, SSP_INTERRUPT_RX_TIMEOUT)) {
        ssp_config_interrupt(SSP0, SSP_INTERRUPT_RX_TIMEOUT, DISABLE);
        while (ssp_get_flag_status(SSP0, SSP_FLAG_RX_FIFO_NOT_EMPTY)) {
            spi0_rx_data[rx_idx_spi0++] = SSP0->DR;
        }
        ssp_clear_interrupt(SSP0, SSP_INTERRUPT_RX_TIMEOUT);
        ssp_config_interrupt(SSP0, SSP_INTERRUPT_RX_TIMEOUT, ENABLE);
    }
}

void ssp1_IRQHandler(void)
{
    if (ssp_get_interrupt_status(SSP1, SSP_INTERRUPT_TX_FIFO_TRIGGER)) {
        ssp_config_interrupt(SSP1, SSP_INTERRUPT_TX_FIFO_TRIGGER, DISABLE);
        ssp_clear_interrupt(SSP1, SSP_INTERRUPT_TX_FIFO_TRIGGER);
        ssp_config_interrupt(SSP1, SSP_INTERRUPT_TX_FIFO_TRIGGER, ENABLE);
    }
    if (ssp_get_interrupt_status(SSP1, SSP_INTERRUPT_RX_FIFO_TRIGGER)) {
        ssp_config_interrupt(SSP1, SSP_INTERRUPT_RX_FIFO_TRIGGER, DISABLE);
        while (ssp_get_flag_status(SSP1, SSP_FLAG_RX_FIFO_NOT_EMPTY)) {
            spi0_rx_data[rx_idx_spi1++] = SSP1->DR;
        }
        ssp_clear_interrupt(SSP1, SSP_INTERRUPT_RX_FIFO_TRIGGER);
        ssp_config_interrupt(SSP1, SSP_INTERRUPT_RX_FIFO_TRIGGER, ENABLE);
    }
    if (ssp_get_interrupt_status(SSP1, SSP_INTERRUPT_RX_FIFO_OVERRUN)) {
        ssp_config_interrupt(SSP1, SSP_INTERRUPT_RX_FIFO_OVERRUN, DISABLE);
        ssp_clear_interrupt(SSP1, SSP_INTERRUPT_RX_FIFO_OVERRUN);
        ssp_config_interrupt(SSP1, SSP_INTERRUPT_RX_FIFO_OVERRUN, ENABLE);
    }
    if (ssp_get_interrupt_status(SSP1, SSP_INTERRUPT_RX_TIMEOUT)) {
        ssp_config_interrupt(SSP1, SSP_INTERRUPT_RX_TIMEOUT, DISABLE);
        while (ssp_get_flag_status(SSP1, SSP_FLAG_RX_FIFO_NOT_EMPTY)) {
            spi0_rx_data[rx_idx_spi1++] = SSP1->DR;
        }
        ssp_clear_interrupt(SSP1, SSP_INTERRUPT_RX_TIMEOUT);
        ssp_config_interrupt(SSP1, SSP_INTERRUPT_RX_TIMEOUT, ENABLE);
    }
}

void ssp2_IRQHandler(void)
{
    if (ssp_get_interrupt_status(SSP2, SSP_INTERRUPT_TX_FIFO_TRIGGER)) {
        ssp_config_interrupt(SSP2, SSP_INTERRUPT_TX_FIFO_TRIGGER, DISABLE);
        ssp_clear_interrupt(SSP2, SSP_INTERRUPT_TX_FIFO_TRIGGER);
        ssp_config_interrupt(SSP2, SSP_INTERRUPT_TX_FIFO_TRIGGER, ENABLE);
    }
    if (ssp_get_interrupt_status(SSP2, SSP_INTERRUPT_RX_FIFO_TRIGGER)) {
        ssp_config_interrupt(SSP2, SSP_INTERRUPT_RX_FIFO_TRIGGER, DISABLE);
        while (ssp_get_flag_status(SSP2, SSP_FLAG_RX_FIFO_NOT_EMPTY)) {
            spi0_rx_data[rx_idx_spi2++] = SSP2->DR;
        }
        ssp_clear_interrupt(SSP2, SSP_INTERRUPT_RX_FIFO_TRIGGER);
        ssp_config_interrupt(SSP2, SSP_INTERRUPT_RX_FIFO_TRIGGER, ENABLE);
    }
    if (ssp_get_interrupt_status(SSP2, SSP_INTERRUPT_RX_FIFO_OVERRUN)) {
        ssp_config_interrupt(SSP2, SSP_INTERRUPT_RX_FIFO_OVERRUN, DISABLE);
        ssp_clear_interrupt(SSP2, SSP_INTERRUPT_RX_FIFO_OVERRUN);
        ssp_config_interrupt(SSP2, SSP_INTERRUPT_RX_FIFO_OVERRUN, ENABLE);
    }
    if (ssp_get_interrupt_status(SSP2, SSP_INTERRUPT_RX_TIMEOUT)) {
        ssp_config_interrupt(SSP2, SSP_INTERRUPT_RX_TIMEOUT, DISABLE);
        while (ssp_get_flag_status(SSP2, SSP_FLAG_RX_FIFO_NOT_EMPTY)) {
            spi0_rx_data[rx_idx_spi2++] = SSP2->DR;
        }
        ssp_clear_interrupt(SSP2, SSP_INTERRUPT_RX_TIMEOUT);
        ssp_config_interrupt(SSP2, SSP_INTERRUPT_RX_TIMEOUT, ENABLE);
    }
}

int main(void)
{
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOB, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOD, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SSP0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SSP1, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SSP2, true);

    gpio_set_iomux(GPIOA, GPIO_PIN_0, 4); // CLK:GP0
    gpio_set_iomux(GPIOA, GPIO_PIN_1, 4); // NSS:GP1
    gpio_set_iomux(GPIOA, GPIO_PIN_2, 4); // TX:GP2
    gpio_set_iomux(GPIOA, GPIO_PIN_3, 4); // RX:GP3

    spi0_tx_rx();

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
