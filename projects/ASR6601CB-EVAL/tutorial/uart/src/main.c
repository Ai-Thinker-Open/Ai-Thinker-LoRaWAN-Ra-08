#include <string.h>

#include "init.h"
#include "tremo_delay.h"


/**
 * @brief Setup the board,enable GPIO
 */
void setup_board()
{
    const rcc_peripheral_t peripheral_array[] = {
        RCC_PERIPHERAL_UART2,
        RCC_PERIPHERAL_GPIOA,
        RCC_PERIPHERAL_GPIOB
        };
    board_init(peripheral_array, sizeof(peripheral_array) / sizeof(rcc_peripheral_t));
}



int main(void)
{
    setup_board();
    uart_log_init();
    printf("start test light\n");
    uart_send_data(UART2,0x01);
    uart_send_data(UART2,0x01);
    uart_send_data(UART2,0x01);
    uart_send_data(UART2,0x01);
    while (1)
    {
       uint8_t rec =  uart_receive_data(UART2);
       printf("receive data %d",rec);
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
