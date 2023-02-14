/*
 * @Author: your name
 * @Date: 2022-04-21 21:49:45
 * @LastEditTime: 2022-04-26 16:47:17
 * @LastEditors: Please set LastEditors
 * @Description: 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * @FilePath: \LoRaWAN-Ra-08\projects\ASR6601CB-EVAL\examples\gptimer\simple_timer\src\main.c
 */
#include <stdio.h>
#include <stdarg.h>
#include "tremo_rcc.h"
#include "tremo_timer.h"

#include "tremo_uart.h"
#include "tremo_gpio.h"
#include "tremo_rcc.h"
#include "lora_net.h"
#include "lora_driver.h"

typedef struct KEY_STRUCT
{
    u8 key_status;      /*按键状态*/
    int key_press_time; /*按键按下时间*/
    u8 key_lock;        /*按键锁*/
    u8 key_num;         /*按键按下计数*/
};

struct KEY_STRUCT key = {0, 0, 0, 0};

gpio_t *g_test_gpiox = GPIOA;
uint8_t g_test_pin = GPIO_PIN_2;

void gptimer_simple_timer(timer_gp_t *TIMERx)
{
    timer_init_t timerx_init;

    timer_config_interrupt(TIMERx, TIMER_DIER_UIE, ENABLE);

    timerx_init.prescaler = 23999; // sysclock defaults to 24M, is divided by (prescaler + 1) to 1k
    timerx_init.counter_mode = TIMER_COUNTERMODE_UP;
    timerx_init.period = 50; // time period is ((1 / 1k) * 1000)
    timerx_init.clock_division = TIMER_CKD_FPCLK_DIV1;
    timerx_init.autoreload_preload = false;
    timer_init(TIMERx, &timerx_init);

    timer_generate_event(TIMERx, TIMER_EGR_UG, ENABLE);
    timer_clear_status(TIMERx, TIMER_SR_UIF);

    timer_cmd(TIMERx, true); //
}

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

    uart_config_interrupt(CONFIG_DEBUG_UART, UART_INTERRUPT_RX_DONE, ENABLE);
    uart_config_interrupt(CONFIG_DEBUG_UART, UART_INTERRUPT_RX_TIMEOUT, ENABLE);

    NVIC_EnableIRQ(UART0_IRQn);
}

void gptim0_IRQHandler(void)
{
    bool state;
    timer_get_status(TIMER2, TIMER_SR_UIF, &state);
    if (state)
    {
        if (key.key_lock)
        {
            key.key_press_time++;
        }
        timer_clear_status(TIMER2, TIMER_SR_UIF);
    }
}
extern uint8_t sendMsgFlag;
void GPIO_IRQHandler(void)
{
    if (gpio_get_interrupt_status(GPIOA, GPIO_PIN_2))
    {

        if (key.key_lock == 1)
        {
            if (key.key_press_time > 1 && key.key_press_time < 10)
            {
                printf(" Short Press\r\n");

                gpio_write(GPIOA, GPIO_PIN_4, GPIO_LEVEL_LOW);
                gpio_write(GPIOA, GPIO_PIN_5, GPIO_LEVEL_HIGH);

                key.key_press_time = 0;

                if (2 == sendMsgFlag)
                {
                    sendMsgFlag = 0;
                }
                else
                {
                    printf("[%s()-%d]lora busy\r\n", __func__, __LINE__);
                }
            }

            if (key.key_press_time > 30)
            {

                gpio_write(GPIOA, GPIO_PIN_4, GPIO_LEVEL_HIGH);
                gpio_write(GPIOA, GPIO_PIN_5, GPIO_LEVEL_LOW);

                printf("Long Press \r\n");
                key.key_lock = 1;
                key.key_press_time = 0;
            }

            key.key_lock = 0;
        }
        else
        {
            key.key_lock = 1;
            key.key_press_time = 0;
        }
    }
    gpio_clear_interrupt(GPIOA, GPIO_PIN_2);
}

int main(void)
{

    rcc_enable_oscillator(RCC_OSC_XO32K, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_TIMER2, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_UART0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOB, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOD, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_PWR, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_RTC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SAC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_LORA, true);

    gpio_init(g_test_gpiox, g_test_pin, GPIO_MODE_INPUT_PULL_DOWN);
    gpio_config_interrupt(g_test_gpiox, g_test_pin, GPIO_INTR_RISING_FALLING_EDGE);

    delay_ms(100);
    pwr_xo32k_lpm_cmd(true);

    uart_log_init();
    RtcInit();
    gptimer_simple_timer(TIMER2);

    /* NVIC config */
    NVIC_EnableIRQ(GPIO_IRQn);
    NVIC_SetPriority(GPIO_IRQn, 2);

    NVIC_EnableIRQ(TIMER2_IRQn);
    NVIC_SetPriority(TIMER2_IRQn, 3);

    // 初始化GPIO
    gpio_set_iomux(GPIOA, GPIO_PIN_4, 0);
    gpio_set_iomux(GPIOA, GPIO_PIN_5, 0);
    gpio_set_iomux(GPIOA, GPIO_PIN_7, 0);
    gpio_init(GPIOA, GPIO_PIN_4, GPIO_MODE_OUTPUT_PP_LOW);
    gpio_init(GPIOA, GPIO_PIN_5, GPIO_MODE_OUTPUT_PP_LOW);
    gpio_init(GPIOA, GPIO_PIN_7, GPIO_MODE_OUTPUT_PP_LOW);

#ifdef CONFIG_GATEWAY
    printf("GateWay init OK \r\n");
#else
    printf("Node init OK\r\n");
#endif

    Ra08KitLoraTestStart();
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
