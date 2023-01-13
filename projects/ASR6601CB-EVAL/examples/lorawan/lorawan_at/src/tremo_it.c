#include "tremo_lpuart.h"
#include "tremo_it.h"

extern void RadioOnDioIrq(void);
extern void RtcOnIrq(void);
extern void linkwan_serial_input(uint8_t cmd);
extern void dma0_IRQHandler(void);
extern void dma1_IRQHandler(void);
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

/**
 * @brief  This function handles PPP interrupt request.
 * @param  None
 * @retval None
 */
void LORA_IRQHandler()
{
    RadioOnDioIrq();
}

void RTC_IRQHandler(void)
{
    RtcOnIrq();
}

void UART0_IRQHandler(void)
{
}

void LPUART_IRQHandler(void)
{
    if (lpuart_get_rx_status(LPUART, LPUART_SR0_RX_DONE_STATE)) {
        uint8_t rx_data_temp = lpuart_receive_data(LPUART);
        lpuart_clear_rx_status(LPUART, LPUART_SR0_RX_DONE_STATE);
#ifdef CONFIG_LWAN_AT
        linkwan_serial_input(rx_data_temp);
#endif
    }
}
/**
 * @brief  This function handles dma0 Handler.
 * @param  None
 * @retval None
 */
void DMA0_IRQHandler(void)
{
    dma0_IRQHandler();
}

/**
 * @brief  This function handles dma1 Handler.
 * @param  None
 * @retval None
 */
void DMA1_IRQHandler(void)
{
    dma1_IRQHandler();
}