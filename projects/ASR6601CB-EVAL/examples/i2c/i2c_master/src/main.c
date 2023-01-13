#include <stdio.h>
#include "tremo_rcc.h"
#include "tremo_gpio.h"
#include "tremo_i2c.h"

int main(void)
{
    uint32_t slave_addr = 0x76;
    uint8_t data[10] = {0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9};
    i2c_config_t config;

    // enable the clk
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_I2C0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);

    // set iomux
    gpio_set_iomux(GPIOA, GPIO_PIN_14, 3);
    gpio_set_iomux(GPIOA, GPIO_PIN_15, 3);

    // init
    i2c_config_init(&config);
    i2c_init(I2C0, &config);
    i2c_cmd(I2C0, true);

    // start
    i2c_master_send_start(I2C0, slave_addr, I2C_WRITE);
	
    i2c_clear_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY);
    while (i2c_get_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY) != SET)
        ;

    // send data
    for(int i=0; i<10; i++) {
        i2c_send_data(I2C0, data[i]);

        i2c_clear_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY);
        while (i2c_get_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY) != SET)
            ;
    }

    // stop
    i2c_master_send_stop(I2C0);

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
