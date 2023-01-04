#include <stdio.h>
#include "dht11.h"
#include "init.h"

#define DTH11_PIN GPIO_PIN_6

void setup_board()
{
    const rcc_peripheral_t peripheral_array[] = {
        RCC_PERIPHERAL_UART0, 
        RCC_PERIPHERAL_GPIOA,
        RCC_PERIPHERAL_GPIOB};
    board_init(peripheral_array, sizeof(peripheral_array) / sizeof(rcc_peripheral_t));

}

int main(void)
{
    setup_board();
    gpio_set_iomux(GPIOA,DTH11_PIN,0);
    gpio_write(GPIOA,DTH11_PIN,GPIO_LEVEL_HIGH);
    uart_log_init();
    delay_init();
    
    printf("start read temp and hum\n");
    delay_ms(1000*5);

    while (1) { 
        dht11_setup(DTH11_PIN);
        dht11_read();
        delay_ms(1000*20);
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(void* file, uint32_t line)
{
    (void)file;
    (void)line;

    while (1) { }
}
#endif
