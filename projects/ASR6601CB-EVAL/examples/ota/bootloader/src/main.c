#include <stdio.h>
#include "bootloader.h"
#include "tremo_wdg.h"
#include "tremo_delay.h"
#include "tremo_uart.h"
#include "tremo_gpio.h"
#include "tremo_rcc.h"

void jumpToApp(int addr)
{
    __asm("LDR SP, [R0]");
    __asm("LDR PC, [R0, #4]");
}

//bootloader mode select, just an example
int boot_mode_sel(void)
{
    uint8_t input_value = 0;
    
    input_value = gpio_read(BOOT_MODE_GPIOX, BOOT_MODE_GPIO_PIN);
    if (BOOT_MODE_NO_JUMP == input_value) {
        delay_ms(1); //debounce
        input_value = gpio_read(BOOT_MODE_GPIOX, BOOT_MODE_GPIO_PIN);
        if(BOOT_MODE_NO_JUMP == input_value)
            return BOOT_MODE_NO_JUMP;
    } else {
        delay_ms(1); //debounce
        input_value = gpio_read(BOOT_MODE_GPIOX, BOOT_MODE_GPIO_PIN);
        if(BOOT_MODE_JUMP2APP == input_value)
            return BOOT_MODE_JUMP2APP;
    }
    
    return BOOT_MODE_NO_JUMP;
}

void boot_to_app(uint32_t addr)
{   
    SysTick->CTRL  &= ~SysTick_CTRL_ENABLE_Msk; 

    TREMO_REG_WR(CM4_IRQ_CLR, 0xFFFFFFFF); // close interrupts
    TREMO_REG_WR(CM4_IRQ_VECT_BASE, addr); // set VTOR
    jumpToApp(addr);
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
}

int main(void)
{
    int mode_sel;

    rcc_enable_oscillator(RCC_OSC_XO32K, true);
	
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOB, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOD, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_UART0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SYSCFG, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SEC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_CRC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_RTC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SAC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_LORA, true);

    uart_log_init();
	
    RtcInit();

    //init button
    gpio_set_iomux(BOOT_MODE_GPIOX, BOOT_MODE_GPIO_PIN, 0);
    gpio_init(BOOT_MODE_GPIOX, BOOT_MODE_GPIO_PIN, GPIO_MODE_INPUT_PULL_UP); 

    mode_sel = boot_mode_sel();
    if ((BOOT_MODE_NO_JUMP == mode_sel) || 
        (SYSCFG->CR4 & BOOT_MODE_REG_BIT) ||
        (*(volatile uint32_t *)(APP_START_ADDR) == *(volatile uint32_t *)(APP_START_ADDR+4))) {
        SYSCFG->CR4 &= ~(BOOT_MODE_REG_BIT);

        boot_handle_cmd();
    } else {
        //boot to user image
        boot_to_app(APP_START_ADDR);
    }

    while(1);
}

