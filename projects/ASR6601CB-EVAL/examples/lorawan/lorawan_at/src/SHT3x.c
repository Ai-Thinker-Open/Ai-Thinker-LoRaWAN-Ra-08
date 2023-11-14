#include <stdio.h>

// #include <FreeRTOS.h>
// #include <task.h>

// #include <hosal_i2c.h>
// #include <bl_gpio.h>
// #include <blog.h>
#include "tremo_i2c.h"
#include "tremo_gpio.h"
#include "tremo_rcc.h"

#define SHT31_DEFAULT_ADDR 0x0044
#define SHT31_MEAS_HIGHREP 0x2400

#pragma pack(1)
struct sht3x_data
{
    uint8_t st_high;
    uint8_t st_low;
    uint8_t st_crc8;
    uint8_t srh_high;
    uint8_t srh_low;
    uint8_t srh_crc8;
};
#pragma pack()

static uint8_t crc8(uint8_t* data, int len)
{
    const uint8_t POLYNOMIAL = 0x31;
    uint8_t crc = 0xFF;
    for (int j = len; j; --j)
    {
        crc ^= *data++;
        for (int i = 8; i; --i)
        {
            crc = (crc & 0x80)
                ? (crc << 1) ^ POLYNOMIAL
                : (crc << 1);
        }
    }
    return crc;
}
uint8_t sht3x_ReadTempHumi(int32_t *i32_temp, uint32_t *u32_tempPoint, uint32_t *u32_humi)
{
    struct sht3x_data data;
    uint32_t Timeout_us = 0;
    uint8_t command[2] = { SHT31_MEAS_HIGHREP >> 8, SHT31_MEAS_HIGHREP & 0xff };

        // start
        i2c_master_send_start(I2C0, SHT31_DEFAULT_ADDR, I2C_WRITE);
        i2c_clear_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY);
        Timeout_us = 0;
        while (i2c_get_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY) != SET){
            if(Timeout_us > 1000*100){
                printf("i2c error1\r\n");
                return 1;
            }
            delay_us(1);
            Timeout_us++;
        }

        // write data
        i2c_send_data(I2C0, command[0]);
        i2c_clear_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY);
        Timeout_us = 0;
        while (i2c_get_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY) != SET){
            if(Timeout_us > 1000*100){
                printf("i2c error2\r\n");
                return 2;
            }
            delay_us(1);
            Timeout_us++;
        }
        i2c_send_data(I2C0, command[1]);
        i2c_clear_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY);
        Timeout_us = 0;
        while (i2c_get_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY) != SET){
            if(Timeout_us > 1000*100){
                printf("i2c error3\r\n");
                return 3;
            }
            delay_us(1);
            Timeout_us++;
        }

        // restart
        i2c_master_send_start(I2C0, SHT31_DEFAULT_ADDR, I2C_READ);
        i2c_clear_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY);
        Timeout_us = 0;
        while (i2c_get_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY) != SET){
            if(Timeout_us > 1000*100){
                printf("i2c error4\r\n");
                return 4;
            }
            delay_us(1);
            Timeout_us++;
        }

        // read data
        i2c_set_receive_mode(I2C0, I2C_ACK);
        Timeout_us = 0;
        while (i2c_get_flag_status(I2C0, I2C_FLAG_RECV_FULL) != SET){
            if(Timeout_us > 1000*100){
                printf("i2c error5\r\n");
                return 5;
            }
            delay_us(1);
            Timeout_us++;
        }
        i2c_clear_flag_status(I2C0, I2C_FLAG_RECV_FULL);
        data.st_high = i2c_receive_data(I2C0);
        i2c_set_receive_mode(I2C0, I2C_ACK);
        Timeout_us = 0;
        while (i2c_get_flag_status(I2C0, I2C_FLAG_RECV_FULL) != SET){
            if(Timeout_us > 1000*100){
                printf("i2c error6\r\n");
                return 6;
            }
            delay_us(1);
            Timeout_us++;
        }
        i2c_clear_flag_status(I2C0, I2C_FLAG_RECV_FULL);
        data.st_low = i2c_receive_data(I2C0);
        i2c_set_receive_mode(I2C0, I2C_ACK);
        Timeout_us = 0;
        while (i2c_get_flag_status(I2C0, I2C_FLAG_RECV_FULL) != SET){
            if(Timeout_us > 1000*100){
                printf("i2c error7\r\n");
                return 7;
            }
            delay_us(1);
            Timeout_us++;
        }
        i2c_clear_flag_status(I2C0, I2C_FLAG_RECV_FULL);
        data.st_crc8 = i2c_receive_data(I2C0);
        i2c_set_receive_mode(I2C0, I2C_ACK);
        while (i2c_get_flag_status(I2C0, I2C_FLAG_RECV_FULL) != SET){
            if(Timeout_us > 1000*100){
                printf("i2c error8\r\n");
                return 8;
            }
            delay_us(1);
            Timeout_us++;
        }
        i2c_clear_flag_status(I2C0, I2C_FLAG_RECV_FULL);
        data.srh_high = i2c_receive_data(I2C0);
        i2c_set_receive_mode(I2C0, I2C_ACK);
        Timeout_us = 0;
        while (i2c_get_flag_status(I2C0, I2C_FLAG_RECV_FULL) != SET){
            if(Timeout_us > 1000*100){
                printf("i2c error9\r\n");
                return 9;
            }
            delay_us(1);
            Timeout_us++;
        }
        i2c_clear_flag_status(I2C0, I2C_FLAG_RECV_FULL);
        data.srh_low = i2c_receive_data(I2C0);
        i2c_set_receive_mode(I2C0, I2C_NAK);
        Timeout_us = 0;
        while (i2c_get_flag_status(I2C0, I2C_FLAG_RECV_FULL) != SET){
            if(Timeout_us > 1000*100){
                printf("i2c error10\r\n");
                return 10;
            }
            delay_us(1);
            Timeout_us++;
        }
        i2c_clear_flag_status(I2C0, I2C_FLAG_RECV_FULL);
        data.srh_crc8 = i2c_receive_data(I2C0);

        // stop
        i2c_master_send_stop(I2C0);

        char temperature_str[8];
        char humidity_str[8];

        if (crc8(&data.st_high, 2) == data.st_crc8) {
            uint16_t st = data.st_high;
            st <<= 8;
            st |= data.st_low;

            int temp = st;
            temp *= 17500;
            temp /= 0xffff;
            temp = -4500 + temp;

            int temperature_integer = temp / 100;

            if (temp < 0) {
                temp = -temp;
            }

            unsigned temperature_decimal = temp % 100;

            *i32_temp = temperature_integer;
            *u32_tempPoint = temperature_decimal;
            sprintf(temperature_str, "%d.%02u C", temperature_integer, temperature_decimal);
        }
        else {
            sprintf(temperature_str, "%s", "N/A C");
            return 11;
        }

        if (crc8(&data.srh_high, 2) == data.srh_crc8) {
            uint16_t srh = data.srh_high;
            srh <<= 8;
            srh |= data.srh_low;

            unsigned humidity = srh;
            humidity *= 10000;
            humidity /= 0xFFFF;

            unsigned humidity_integer = humidity / 100;

            *u32_humi = humidity_integer;
            sprintf(humidity_str, "%u %%", humidity_integer);
        }
        else {
            sprintf(humidity_str, "N/A %%");
            return 12;
        }

        // printf("temperature: %s\thumidity: %s\r\n", temperature_str, humidity_str);
        return 0;
}
int sht3x_init(void)
{
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

}
int sht3x_test(void)
{
    uint32_t Timeout_us = 0;
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


    for (;;) {

        struct sht3x_data data;

        uint8_t command[2] = { SHT31_MEAS_HIGHREP >> 8, SHT31_MEAS_HIGHREP & 0xff };

        // start
        i2c_master_send_start(I2C0, SHT31_DEFAULT_ADDR, I2C_WRITE);
        i2c_clear_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY);
        Timeout_us = 0;
        while (i2c_get_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY) != SET){
            if(Timeout_us > 1000*100){
                printf("i2c error1\r\n");
            }
            delay_us(1);
            Timeout_us++;
        }

        // write data
        i2c_send_data(I2C0, command[0]);
        i2c_clear_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY);
        Timeout_us = 0;
        while (i2c_get_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY) != SET){
            if(Timeout_us > 1000*100){
                printf("i2c error1\r\n");
            }
            delay_us(1);
            Timeout_us++;
        }
        i2c_send_data(I2C0, command[1]);
        i2c_clear_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY);
        Timeout_us = 0;
        while (i2c_get_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY) != SET){
            if(Timeout_us > 1000*100){
                printf("i2c error1\r\n");
            }
            delay_us(1);
            Timeout_us++;
        }

        // restart
        i2c_master_send_start(I2C0, SHT31_DEFAULT_ADDR, I2C_READ);
        i2c_clear_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY);
        Timeout_us = 0;
        while (i2c_get_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY) != SET){
            if(Timeout_us > 1000*100){
                printf("i2c error1\r\n");
            }
            delay_us(1);
            Timeout_us++;
        }

        // read data
        i2c_set_receive_mode(I2C0, I2C_ACK);
        Timeout_us = 0;
        while (i2c_get_flag_status(I2C0, I2C_FLAG_RECV_FULL) != SET){
            if(Timeout_us > 1000*100){
                printf("i2c error1\r\n");
            }
            delay_us(1);
            Timeout_us++;
        }
        i2c_clear_flag_status(I2C0, I2C_FLAG_RECV_FULL);
        data.st_high = i2c_receive_data(I2C0);
        i2c_set_receive_mode(I2C0, I2C_ACK);
        Timeout_us = 0;
        while (i2c_get_flag_status(I2C0, I2C_FLAG_RECV_FULL) != SET){
            if(Timeout_us > 1000*100){
                printf("i2c error1\r\n");
            }
            delay_us(1);
            Timeout_us++;
        }
        i2c_clear_flag_status(I2C0, I2C_FLAG_RECV_FULL);
        data.st_low = i2c_receive_data(I2C0);
        i2c_set_receive_mode(I2C0, I2C_ACK);
        Timeout_us = 0;
        while (i2c_get_flag_status(I2C0, I2C_FLAG_RECV_FULL) != SET){
            if(Timeout_us > 1000*100){
                printf("i2c error1\r\n");
            }
            delay_us(1);
            Timeout_us++;
        }
        i2c_clear_flag_status(I2C0, I2C_FLAG_RECV_FULL);
        data.st_crc8 = i2c_receive_data(I2C0);
        i2c_set_receive_mode(I2C0, I2C_ACK);
        while (i2c_get_flag_status(I2C0, I2C_FLAG_RECV_FULL) != SET){
            if(Timeout_us > 1000*100){
                printf("i2c error1\r\n");
            }
            delay_us(1);
            Timeout_us++;
        }
        i2c_clear_flag_status(I2C0, I2C_FLAG_RECV_FULL);
        data.srh_high = i2c_receive_data(I2C0);
        i2c_set_receive_mode(I2C0, I2C_ACK);
        Timeout_us = 0;
        while (i2c_get_flag_status(I2C0, I2C_FLAG_RECV_FULL) != SET){
            if(Timeout_us > 1000*100){
                printf("i2c error1\r\n");
            }
            delay_us(1);
            Timeout_us++;
        }
        i2c_clear_flag_status(I2C0, I2C_FLAG_RECV_FULL);
        data.srh_low = i2c_receive_data(I2C0);
        i2c_set_receive_mode(I2C0, I2C_NAK);
        Timeout_us = 0;
        while (i2c_get_flag_status(I2C0, I2C_FLAG_RECV_FULL) != SET){
            if(Timeout_us > 1000*100){
                printf("i2c error1\r\n");
            }
            delay_us(1);
            Timeout_us++;
        }
        i2c_clear_flag_status(I2C0, I2C_FLAG_RECV_FULL);
        data.srh_crc8 = i2c_receive_data(I2C0);

        // stop
        i2c_master_send_stop(I2C0);

        char temperature_str[8];
        char humidity_str[8];

        if (crc8(&data.st_high, 2) == data.st_crc8) {
            uint16_t st = data.st_high;
            st <<= 8;
            st |= data.st_low;

            int temp = st;
            temp *= 17500;
            temp /= 0xffff;
            temp = -4500 + temp;

            int temperature_integer = temp / 100;

            if (temp < 0) {
                temp = -temp;
            }

            unsigned temperature_decimal = temp % 100;

            sprintf(temperature_str, "%d.%02u C", temperature_integer, temperature_decimal);
        }
        else {
            sprintf(temperature_str, "%s", "N/A C");
        }

        if (crc8(&data.srh_high, 2) == data.srh_crc8) {
            uint16_t srh = data.srh_high;
            srh <<= 8;
            srh |= data.srh_low;

            unsigned humidity = srh;
            humidity *= 10000;
            humidity /= 0xFFFF;

            unsigned humidity_integer = humidity / 100;

            sprintf(humidity_str, "%u %%", humidity_integer);
        }
        else {
            sprintf(humidity_str, "N/A %%");
        }

        printf("temperature: %s\thumidity: %s\r\n", temperature_str, humidity_str);

        // vTaskDelay(portTICK_RATE_MS * 1000);
        delay_ms(1000);
    }

    return 0;
}
