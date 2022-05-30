#include <stdio.h>
#include <string.h>
#include <math.h>
#include "tremo_dma.h"
#include "tremo_dma_handshake.h"
#include "tremo_bstimer.h"
#include "tremo_rcc.h"
#include "tremo_dac.h"

#define DAC_BUF_LEN 250

uint16_t dac_buf[DAC_BUF_LEN];

/* This example will ouput a 1KHz sine wave on GPIO09 */
int main(void)
{
    dac_config_t config;
    dma_dev_t dma_dev;
	dma_reload_t dma_reload;
    float pi = 3.14;
    float um = 1.5;

    rcc_enable_peripheral_clk(RCC_PERIPHERAL_DAC, true);
	rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);
	rcc_enable_peripheral_clk(RCC_PERIPHERAL_BSTIMER0, true);
	rcc_enable_peripheral_clk(RCC_PERIPHERAL_DMA0, true);
	rcc_enable_peripheral_clk(RCC_PERIPHERAL_SYSCFG, true);

    //sine wave table
    for(int i=0; i<DAC_BUF_LEN; i++){
        dac_buf[i] = (uint16_t)((um*sin((1.0*i/(DAC_BUF_LEN-1))*2*pi)+1.65)*4095/3.3);
    }
    
    memset(&config, 0, sizeof(dac_config_t));
    config.trigger_src = DAC_TRIGGER_SRC_BSTIMER0_TRGO;
    dac_init(&config);

    dac_cmd(true);
    dac_write_data(dac_buf[0]);
	dac_clear_interrupt(DAC_INTR_UNDERFLOW);
	
    dma_dev.dma_num    = 0;
    dma_dev.ch         = 0;
    dma_dev.mode       = M2P_MODE;
    dma_dev.src        = (uint32_t)(dac_buf);
    dma_dev.dest       = (uint32_t)&(DAC->DHR);
    dma_dev.priv       = NULL;
    dma_dev.data_width = 1;
    dma_dev.block_size = DAC_BUF_LEN;
    dma_dev.src_msize  = 0;
    dma_dev.dest_msize = 0;
	dma_dev.handshake  = DMA_HANDSHAKE_DACCTRL;

    dma_init(&dma_dev);
    dma_ch_enable(dma_dev.dma_num, 0);
    dac_dma_cmd(true);
	
	dma_reload.src_reload = true;
	dma_config_reload(&dma_dev, &dma_reload);
    
    //timer
    bstimer_init_t bstimer_init_config;
    bstimer_init_config.bstimer_mms = BSTIMER_MMS_UPDATE;
    bstimer_init_config.period = 1;  
    bstimer_init_config.prescaler = 47;
    bstimer_init_config.autoreload_preload = false;
    bstimer_init(BSTIMER0, &bstimer_init_config);
    bstimer_cmd(BSTIMER0, ENABLE);

    /* Infinite loop */
    while (1) { }
}

#ifdef USE_FULL_ASSERT
void assert_failed(void* file, uint32_t line)
{
    (void)file;
    (void)line;

    while (1) { }
}
#endif
