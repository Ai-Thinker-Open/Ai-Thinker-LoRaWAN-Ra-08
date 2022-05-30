#include <stdio.h>
#include <string.h>
#include "delay.h"
#include "timer.h"
#include "radio.h"
#include "tremo_uart.h"
#include "tremo_gpio.h"
#include "tremo_rcc.h"
#include "tremo_pwr.h"
#include "tremo_wdg.h"
#include "tremo_delay.h"
#include "rtc-board.h"
#include "simple_cmd.h"

extern int tc_lora_test(void);

void uart_log_init(void)
{
    // uart0
    gpio_set_iomux(GPIOB, GPIO_PIN_0, 1);
    gpio_set_iomux(GPIOB, GPIO_PIN_1, 1);

    /* uart config struct init */
    uart_config_t uart_config;
    uart_config_init(&uart_config);

	uart_config.fifo_mode = ENABLE;
	uart_config.mode     = UART_MODE_TXRX;
    uart_config.baudrate = UART_BAUDRATE_115200;
    uart_init(UART0, &uart_config);

	uart_config_interrupt(UART0,UART_INTERRUPT_RX_DONE,ENABLE);
	uart_config_interrupt(UART0,UART_INTERRUPT_RX_TIMEOUT,ENABLE);
	NVIC_SetPriority(UART_INTERRUPT_RX_DONE, 2);
	NVIC_SetPriority(UART_INTERRUPT_RX_TIMEOUT, 2);
	NVIC_EnableIRQ(UART0_IRQn);
	
    uart_cmd(UART0, ENABLE);
}

void board_init()
{
    rcc_enable_oscillator(RCC_OSC_XO32K, true);

    rcc_enable_peripheral_clk(RCC_PERIPHERAL_UART0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOB, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOD, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_PWR, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_RTC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_LORA, true);
    
    delay_ms(100);
    pwr_xo32k_lpm_cmd(true);
    
    uart_log_init();

    RtcInit();
}

int main(void)
{
    // Target board initialization
    board_init();
	//开启看门狗
	rcc_enable_peripheral_clk(RCC_PERIPHERAL_WDG, true);
	uint32_t timeout      = 5000;	//看门狗复位事件(ms)
	uint32_t wdgclk_freq  = rcc_get_clk_freq(RCC_PCLK0);
	uint32_t reload_value = timeout * (wdgclk_freq / 1000 / 2);
	// start wdg
	wdg_start(reload_value);
	NVIC_EnableIRQ(WDG_IRQn);

	SimpleCmdLoadConfig();//加载flash

    printf("\r\n");
	printf("/*******************************************************************\r\n");
	printf("********************************************************************\r\n");
	if(g_simpleCmdConfig.isTestBoard){
		//是测试底板
		printf("*             ASR6601 LoRa factory Test for test board             *\r\n");
		printf("* version:%-57s*\r\n",FACTORY_TEST_VERSION);
		printf("* freq:%-60d*\r\n",g_simpleCmdConfig.testBoardFreq);
		printf("* power:%-59d*\r\n",g_simpleCmdConfig.testBoardPower);
		printf("* ackRssiOffset:%-51d*\r\n",g_simpleCmdConfig.ackRssiOffset);
		switch(g_simpleCmdConfig.testBoardBw){
			case 0:
				printf("* bw:125k                                                          *\r\n");
				break;
			case 1:
				printf("* bw:250k                                                          *\r\n");
				break;
			case 2:
				printf("* bw:500k                                                          *\r\n");
				break;
			default:
				printf("* bw:error(%d)                                                      *\r\n",g_simpleCmdConfig.testBoardBw);
				break;
		}
		printf("* sf:%-62d*\r\n",g_simpleCmdConfig.testBoardSf);
		switch(g_simpleCmdConfig.testBoardCr){
			case 1:
				printf("* cr:[4/5]                                                         *\r\n");
				break;
			case 2:
				printf("* cr:[4/6]                                                         *\r\n");
				break;
			case 3:
				printf("* cr:[4/7]                                                         *\r\n");
				break;
			case 4:
				printf("* cr:[4/8]                                                         *\r\n");
				break;
			default:
				printf("* cr:error(%d)                                                      *\r\n",g_simpleCmdConfig.testBoardCr);
				break;
		}
	}else{
		//是待测模组
		printf("*               ASR6601 LoRa factory Test for module               *\r\n");
		printf("* version:%-57s*\r\n",FACTORY_TEST_VERSION);
	}
	printf("* compile time:%13s-%-38s*\r\n",__DATE__,__TIME__);
	printf("********************************************************************\r\n");
	printf("*******************************************************************/\r\n");
    SimpleCmdStart();
}

#ifdef USE_FULL_ASSERT
void assert_failed(void* file, uint32_t line)
{
    (void)file;
    (void)line;

    while (1) { }
}
#endif