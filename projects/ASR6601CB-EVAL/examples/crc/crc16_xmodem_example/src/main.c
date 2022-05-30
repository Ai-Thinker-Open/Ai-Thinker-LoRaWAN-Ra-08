#include <stdio.h>
#include "tremo_rcc.h"
#include "tremo_crc.h"

static uint8_t test_data[32] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32 };
static uint32_t test_data_size = 32;
static uint32_t test_crc_value = 0x305D;

uint32_t crc16_xmodem(uint8_t* data, uint32_t size)
{
    uint32_t crc_value = 0;

    crc_config_t config;
    config.init_value  = 0;
    config.poly_size   = CRC_POLY_SIZE_16;
    config.poly        = 0x1021;
    config.reverse_in  = CRC_REVERSE_IN_NONE;
    config.reverse_out = false;

    crc_init(&config);

    crc_value = crc_calc8(data, size);

    return crc_value;
}

int main(void)
{
    uint32_t crc_value = 0;
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_CRC, true);

    crc_value = crc16_xmodem(test_data, test_data_size);
    if (crc_value != test_crc_value) {
        // error
        while (1)
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
