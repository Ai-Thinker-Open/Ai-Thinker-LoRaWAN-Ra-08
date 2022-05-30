#include <stdio.h>
#include "tremo_flash.h"

#define TEST_DATA_SIZE 4096

static uint8_t test_data[TEST_DATA_SIZE];
static uint32_t test_addr = 0x0800D000;

int main(void)
{
    for (int i = 0; i < TEST_DATA_SIZE; i++) {
        test_data[i] = (i & 0xFF);
    }

    flash_erase_page(test_addr);

    for (int i = 0; i < TEST_DATA_SIZE; i += FLASH_LINE_SIZE)
        flash_program_line(test_addr + i, test_data + i);

    for (int i = 0; i < TEST_DATA_SIZE; i++) {
        if (*(uint8_t*)(test_addr + i) != (i & 0xFF)) {
            // error
            while (1)
                ;
        }
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
