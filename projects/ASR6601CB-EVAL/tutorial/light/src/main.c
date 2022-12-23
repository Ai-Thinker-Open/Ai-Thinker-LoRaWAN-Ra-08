#include <string.h>

#include "init.h"
#include "tremo_delay.h"

#define LIGHT_WARM GPIO_PIN_15
#define LIGHT_COLD GPIO_PIN_14

/**
 * @brief Setup the board,enable GPIO
 */
void setup_board()
{
    const rcc_peripheral_t peripheral_array[] = {
        RCC_PERIPHERAL_UART0, RCC_PERIPHERAL_GPIOA,
        RCC_PERIPHERAL_GPIOB};
    board_init(peripheral_array, sizeof(peripheral_array) / sizeof(rcc_peripheral_t));
}

/**
 * @brief Setup the warm and colde light,which will off
 *
 */
void setup_light()
{
    gpio_init(GPIOA, LIGHT_COLD, GPIO_MODE_OUTPUT_PP_LOW);
    gpio_init(GPIOA, LIGHT_WARM, GPIO_MODE_OUTPUT_PP_LOW);
}

/**
 * @brief turn warm light on
 * 
 */
void turn_warm_light_on()
{
    gpio_write(GPIOA, LIGHT_COLD, GPIO_LEVEL_LOW);
    gpio_write(GPIOA, LIGHT_WARM, GPIO_LEVEL_HIGH);
}

/**
 * @brief turn cold light on
 * 
 */
void turn_cold_light_on()
{
    gpio_write(GPIOA, LIGHT_COLD, GPIO_LEVEL_HIGH);
    gpio_write(GPIOA, LIGHT_WARM, GPIO_LEVEL_LOW);
}

int main(void)
{
    setup_board();
    uart_log_init();
    printf("start test light\n");
    setup_light();
    //loop , cold and warm light on or off
    while (1)
    {
        turn_cold_light_on();
        delay_ms(1000);
        turn_warm_light_on();
        delay_ms(1000);
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
