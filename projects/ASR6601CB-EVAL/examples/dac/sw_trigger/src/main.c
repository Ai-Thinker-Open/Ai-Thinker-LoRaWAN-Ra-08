#include <stdio.h>
#include <string.h>
#include "tremo_rcc.h"
#include "tremo_dac.h"


int main(void)
{
    dac_config_t config;

    rcc_enable_peripheral_clk(RCC_PERIPHERAL_DAC, true);
	rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);

    memset(&config, 0, sizeof(dac_config_t));
    config.trigger_src = DAC_TRIGGER_SRC_SOFTWARE;

    dac_init(&config);
    dac_cmd(true);
    
    dac_write_data(4095);
    dac_software_trigger_cmd(true);

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
