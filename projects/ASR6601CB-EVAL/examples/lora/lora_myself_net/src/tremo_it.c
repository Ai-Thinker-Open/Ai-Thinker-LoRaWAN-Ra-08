#include "stdio.h"
#include "tremo_it.h"
#include "tremo_gpio.h"
#include "tremo_uart.h"
#include "lora_config.h"
extern void RadioOnDioIrq(void);
extern void RtcOnIrq(void);

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
    while (1)
    {
    }
}

/**
 * @brief  This function handles Memory Manage exception.
 * @param  None
 * @retval None
 */
void MemManage_Handler(void)
{
    /* Go to infinite loop when Memory Manage exception occurs */
    while (1)
    {
    }
}

/**
 * @brief  This function handles Bus Fault exception.
 * @param  None
 * @retval None
 */
void BusFault_Handler(void)
{
    /* Go to infinite loop when Bus Fault exception occurs */
    while (1)
    {
    }
}

/**
 * @brief  This function handles Usage Fault exception.
 * @param  None
 * @retval None
 */
void UsageFault_Handler(void)
{
    /* Go to infinite loop when Usage Fault exception occurs */
    while (1)
    {
    }
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
}

/**
 * @brief  This function handles PWR Handler.
 * @param  None
 * @retval None
 */
void PWR_IRQHandler()
{
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

void RTC_IRQHandler(void)
{
    RtcOnIrq();
}

/******************************************************************************/
/*                 Tremo Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_cm4.S).                                               */
/******************************************************************************/

/**
 * @brief  This function handles TIMER0 Handler.
 * @param  None
 * @retval None
 */
void TIMER0_IRQHandler(void)
{
    // gptim0_IRQHandler();
}

void TIMER2_IRQHandler()
{
    gptim0_IRQHandler();
}

void UART0_IRQHandler(void)
{
    if (uart_get_interrupt_status(UART0, UART_INTERRUPT_RX_DONE))
    {

        uart_clear_interrupt(UART0, UART_INTERRUPT_RX_DONE);
    }

    if (uart_get_interrupt_status(UART0, UART_INTERRUPT_RX_TIMEOUT))
    {
        uart_clear_interrupt(UART0, UART_INTERRUPT_RX_TIMEOUT);
    }

    while (!uart_get_flag_status(UART0, UART_FLAG_RX_FIFO_EMPTY))
    {

        uint8_t data = uart_receive_data(UART0);
        handler_uart_data(data);
    }
}
