#include "system_cm4.h"
#include "tremo_rcc.h"
#include "tremo_delay.h"

void nvic_init()
{
    NVIC_SetPriority(PendSV_IRQn, (1<<__NVIC_PRIO_BITS) - 1);
    
    for(int i=0; i<=IWDG_IRQn; i++)
        NVIC_SetPriority(i, configLIBRARY_NORMAL_INTERRUPT_PRIORITY);
}

void system_init(void)
{
    // FPU enable
#if (__FPU_PRESENT == 1)
    SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));
#endif

    // enable afec clk
    TREMO_REG_EN(RCC->CGR0, RCC_CGR0_AFEC_CLK_EN_MASK, true);

    // set flash read number to 1
    EFC->TIMING_CFG  =  (EFC->TIMING_CFG & (~0xF0000)) | (1<<16);
    while(!(EFC->SR&0x2));
    
    nvic_init();
	
    delay_init();
}

/********END OF FILE ***********/
