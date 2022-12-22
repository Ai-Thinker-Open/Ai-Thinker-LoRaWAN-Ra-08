#include <string.h>

#include "init.h"

void setup_board()
{
    const rcc_peripheral_t peripheral_array[] = {RCC_PERIPHERAL_UART0, RCC_PERIPHERAL_GPIOA};
    board_init(peripheral_array,sizeof(peripheral_array)/sizeof(rcc_peripheral_t));
}

int main(void)
{
    setup_board();
    printf("start test light");
    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(void *file, uint32_t line)
{
    (void)file;
    (void)line;

    while (1)
    {
    }
}
#endif
