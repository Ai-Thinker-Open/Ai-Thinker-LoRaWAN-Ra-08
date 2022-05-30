#include <stdio.h>
#include <string.h>
#include "tremo_gpio.h"
#include "tremo_it.h"
#include "tremo_uart.h"
#include "simple_cmd.h"

extern gpio_t *g_test_gpiox;
extern uint8_t g_test_pin;
extern void RadioOnDioIrq(void);
volatile uint32_t g_sysTick=0;

/**
 * @brief  This function handles NMI exception.
 * @param  None
 * @retval None
 */
void NMI_Handler(void)
{
}

/**
 * @brief  This function handles Hard Fault exception.
 * @param  None
 * @retval None
 */
void HardFault_Handler(void)
{

    /* Go to infinite loop when Hard Fault exception occurs */
    while (1) { }
}

/**
 * @brief  This function handles Memory Manage exception.
 * @param  None
 * @retval None
 */
void MemManage_Handler(void)
{
    /* Go to infinite loop when Memory Manage exception occurs */
    while (1) { }
}

/**
 * @brief  This function handles Bus Fault exception.
 * @param  None
 * @retval None
 */
void BusFault_Handler(void)
{
    /* Go to infinite loop when Bus Fault exception occurs */
    while (1) { }
}

/**
 * @brief  This function handles Usage Fault exception.
 * @param  None
 * @retval None
 */
void UsageFault_Handler(void)
{
    /* Go to infinite loop when Usage Fault exception occurs */
    while (1) { }
}

/**
 * @brief  This function handles SVCall exception.
 * @param  None
 * @retval None
 */
void SVC_Handler(void)
{
}

/**
 * @brief  This function handles PendSVC exception.
 * @param  None
 * @retval None
 */
void PendSV_Handler(void)
{
}

/**
 * @brief  This function handles SysTick Handler.
 * @param  None
 * @retval None
 */
void SysTick_Handler(void)
{
	g_sysTick++;
}

/**
 * @brief  This function handles PWR Handler.
 * @param  None
 * @retval None
 */
void PWR_IRQHandler()
{
}
//extern int reciveCount;
//extern uint8_t reciveBuf[256];
void UART0_IRQHandler(void)
{
	//printf("[%s()-%d]\r\n",__func__,__LINE__);
	if(uart_get_flag_status(UART0,UART_INTERRUPT_RX_DONE)){
		uart_clear_interrupt(UART0,UART_INTERRUPT_RX_DONE);
	}
	if(uart_get_flag_status(UART0,UART_INTERRUPT_RX_TIMEOUT)){
		uart_clear_interrupt(UART0,UART_INTERRUPT_RX_TIMEOUT);
	}
	while(!uart_get_flag_status(UART0,UART_FLAG_RX_FIFO_EMPTY)){
		uint8_t rx_data_temp = uart_receive_data(UART0);
		if(g_cmdStatus.reciveCount>SIMPLE_CMD_RX_BUF_SIZE-2){
			printf("[%s()-%d]cmd buf overflow\r\n",__func__,__LINE__);
			memset((uint8_t *)g_cmdStatus.reciveBuf,0,SIMPLE_CMD_RX_BUF_SIZE);
			g_cmdStatus.reciveCount=0;
		}
		if(g_cmdStatus.eventFlag & EVENT_CMD_START){
			if('\n'==rx_data_temp){
				printf("\r\ncmd busy\r\n");
			}
		}else{
			g_cmdStatus.reciveBuf[g_cmdStatus.reciveCount]=rx_data_temp;
			g_cmdStatus.reciveCount++;
			if('\n'==rx_data_temp){
				g_cmdStatus.eventFlag|=EVENT_CMD_START;
			}
		}
	}
}

/******************************************************************************/
/*                 Tremo Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_cm4.S).                                               */
/******************************************************************************/

void LORA_IRQHandler()
{
    RadioOnDioIrq();
}

void GPIO_IRQHandler(void)
{
    if (gpio_get_interrupt_status(g_test_gpiox, g_test_pin) == SET) {
        gpio_clear_interrupt(g_test_gpiox, g_test_pin);
    }
}

void WDG_IRQHandler()
{
	printf("watch dog fault\r\n");
}

