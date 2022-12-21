#include <stdio.h>
#include <string.h>

#include "delay.h"
#include "timer.h"
#include "radio.h"
#include "tremo_uart.h"
#include "tremo_gpio.h"
#include "tremo_rcc.h"
#include "tremo_delay.h"
#include "rtc-board.h"

#define LED_RED GPIO_PIN_5
#define LED_GREEN GPIO_PIN_4
#define LED_BLUE GPIO_PIN_7

#define LIGHT_WARM GPIO_PIN_15
#define LIGHT_COLD GPIO_PIN_14

void uart_log_init(void)
{
    // uart0
    gpio_set_iomux(GPIOB, GPIO_PIN_0, 1);
    gpio_set_iomux(GPIOB, GPIO_PIN_1, 1);

    /* uart config struct init */
    uart_config_t uart_config;
    uart_config_init(&uart_config);

    uart_config.baudrate = UART_BAUDRATE_115200;
    uart_init(CONFIG_DEBUG_UART, &uart_config);
    uart_cmd(CONFIG_DEBUG_UART, ENABLE);
}

void board_init()
{
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_UART0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOB, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOD, true);
}

void initLed()
{
    gpio_init(GPIOA, LED_RED, GPIO_MODE_OUTPUT_PP_HIGH);
    gpio_init(GPIOA, LED_GREEN, GPIO_MODE_OUTPUT_PP_HIGH);
    gpio_init(GPIOA, LED_BLUE, GPIO_MODE_OUTPUT_PP_HIGH);
}

void setLedRed()
{
    gpio_write(GPIOA, LED_RED, GPIO_LEVEL_HIGH);
    gpio_write(GPIOA, LED_GREEN, GPIO_LEVEL_LOW);
    gpio_write(GPIOA, LED_BLUE, GPIO_LEVEL_LOW);
}

void setLedGreen()
{
    gpio_write(GPIOA, LED_RED, GPIO_LEVEL_LOW);
    gpio_write(GPIOA, LED_GREEN, GPIO_LEVEL_HIGH);
    gpio_write(GPIOA, LED_BLUE, GPIO_LEVEL_LOW);
}

void setLedBlue()
{
    gpio_write(GPIOA, LED_RED, GPIO_LEVEL_LOW);
    gpio_write(GPIOA, LED_GREEN, GPIO_LEVEL_LOW);
    gpio_write(GPIOA, LED_BLUE, GPIO_LEVEL_HIGH);
}

void initLight()
{
    gpio_init(GPIOA, LIGHT_WARM, GPIO_MODE_OUTPUT_PP_HIGH);
    gpio_init(GPIOA, LIGHT_COLD, GPIO_MODE_OUTPUT_PP_HIGH);
}

void setWarm()
{
    gpio_write(GPIOA, LIGHT_WARM, GPIO_LEVEL_HIGH);
    gpio_write(GPIOA, LIGHT_COLD, GPIO_LEVEL_LOW);
}

void setCold()
{
    gpio_write(GPIOA, LIGHT_COLD, GPIO_LEVEL_HIGH);
    gpio_write(GPIOA, LIGHT_WARM, GPIO_LEVEL_LOW);
}
int main(void)
{
    // Target board initialization
    printf("start led test.\n");
    board_init();
    // initLed();
    // initLight();
    // delay_ms(1000 * 3);
    // while (1)
    // {
    //     setLedRed();
    //     printf("red light\n");
    //     delay_ms(1000);
    //     setLedGreen();
    //     printf("green light\n");
    //     delay_ms(1000);
    //     setLedBlue();
    //     printf("blue light\n");
    //     delay_ms(1000);
    //     // setWarm();
    //     // delay_ms(1000);
    //     // setCold();
    // }
uart_log_init();
    for (int i = 1; i <= 15; i++)
    {
        gpio_init(GPIOA, (i), GPIO_MODE_OUTPUT_PP_HIGH);
        printf("gpio porta now is %d",i);
        delay_ms(1000);
        gpio_write(GPIOA, (i), GPIO_LEVEL_LOW);
        delay_ms(1000);
    }
    for (int i = 1; i <= 15; i++)
    {
        gpio_init(GPIOB, (i), GPIO_MODE_OUTPUT_PP_HIGH);
        printf("gpio portb now is %d",i);
        delay_ms(1000);
        gpio_write(GPIOB, (i), GPIO_LEVEL_LOW);
        delay_ms(1000);
    }
    for (int i = 1; i <= 15; i++)
    {
        gpio_init(GPIOC, (i), GPIO_MODE_OUTPUT_PP_HIGH);
        printf("gpio portc now is %d",i);
        delay_ms(1000);
        gpio_write(GPIOC, (i), GPIO_LEVEL_LOW);
        delay_ms(1000);
    }
    for (int i = 1; i <= 15; i++)
    {
        gpio_init(GPIOD, (i), GPIO_MODE_OUTPUT_PP_HIGH);
        printf("gpio portd now is %d",i);
        delay_ms(1000);
        gpio_write(GPIOD, (i), GPIO_LEVEL_LOW);
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
