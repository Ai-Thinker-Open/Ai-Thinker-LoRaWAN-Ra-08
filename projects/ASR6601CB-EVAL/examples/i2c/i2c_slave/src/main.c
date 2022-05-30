#include <stdio.h>
#include "tremo_rcc.h"
#include "tremo_gpio.h"
#include "tremo_i2c.h"

int main(void)
{
    uint32_t slave_addr = 0x76;
    uint8_t data[10];
    i2c_config_t config;

    // enable the clk
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_I2C0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);

    // set iomux
    gpio_set_iomux(GPIOA, GPIO_PIN_14, 3);
    gpio_set_iomux(GPIOA, GPIO_PIN_15, 3);

    // init
    i2c_config_init(&config);
    config.mode                      = I2C_MODE_SLAVE;
    config.settings.slave.slave_addr = slave_addr;

    i2c_init(I2C0, &config);
    i2c_cmd(I2C0, true);

    // wait slave address detected
    while (i2c_get_flag_status(I2C0, I2C_FLAG_SLAVE_ADDR_DET) != SET)
        ;
    i2c_clear_flag_status(I2C0, I2C_FLAG_SLAVE_ADDR_DET);

    // receive data
    for (int i = 0; i < 10; i++) {
        i2c_set_receive_mode(I2C0, I2C_ACK);

        while (i2c_get_flag_status(I2C0, I2C_FLAG_RECV_FULL) != SET)
            ;

        data[i] = i2c_receive_data(I2C0);

        i2c_clear_flag_status(I2C0, I2C_FLAG_RECV_FULL);
    }

    // wait slave stop detected
    i2c_set_receive_mode(I2C0, I2C_ACK);
    while (i2c_get_flag_status(I2C0, I2C_FLAG_SLAVE_STOP_DET) != SET)
        ;
    i2c_clear_flag_status(I2C0, I2C_FLAG_SLAVE_STOP_DET);

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
