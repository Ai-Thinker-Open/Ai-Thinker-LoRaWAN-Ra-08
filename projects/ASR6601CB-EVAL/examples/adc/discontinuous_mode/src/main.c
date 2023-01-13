#include <stdio.h>
#include "tremo_regs.h"
#include "tremo_adc.h"
#include "tremo_rcc.h"
#include "tremo_gpio.h"

#define ADC_DATA_NUM 100
uint16_t adc_data_1[ADC_DATA_NUM] = {0};
uint16_t adc_data_2[ADC_DATA_NUM] = {0};
float calibrated_sample_1[ADC_DATA_NUM] = {0.0};
float calibrated_sample_2[ADC_DATA_NUM] = {0.0};

void adc_discontinuous_mode_test(void)
{
    uint32_t i;
    gpio_t *gpiox;
    uint32_t pin;
    float gain_value;
    float dco_value;

    adc_get_calibration_value(false, &gain_value, &dco_value);

    gpiox = GPIOA;
    pin = GPIO_PIN_8;
    gpio_init(gpiox, pin, GPIO_MODE_ANALOG);
    pin = GPIO_PIN_11;
    gpio_init(gpiox, pin, GPIO_MODE_ANALOG);

    adc_init();

    adc_config_clock_division(20); //sample frequence 150K

    adc_config_sample_sequence(0, 1);
    adc_config_sample_sequence(1, 2);

    adc_config_conv_mode(ADC_CONV_MODE_DISCONTINUE);

    adc_enable(true);

	adc_start(true);

    while(!adc_get_interrupt_status(ADC_ISR_EOC));
    (void)adc_get_data();

    for (i = 0; i < ADC_DATA_NUM; i++)
    {
        adc_start(true);
        while(!adc_get_interrupt_status(ADC_ISR_EOC));
        adc_data_1[i] = adc_get_data();

        adc_start(true);
        while(!adc_get_interrupt_status(ADC_ISR_EOC));
        adc_data_2[i] = adc_get_data();
    }

    adc_start(false);
    adc_enable(false);

    for (i = 0; i < ADC_DATA_NUM; i++)
    {//calibration sample value
        calibrated_sample_1[i] = ((1.2/4096) * adc_data_1[i] - dco_value) / gain_value;
        calibrated_sample_2[i] = ((1.2/4096) * adc_data_2[i] - dco_value) / gain_value;
    }
}

int main(void)
{
    rcc_set_adc_clk_source(RCC_ADC_CLK_SOURCE_RCO48M);

    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOB, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOD, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_ADC, true);

    adc_discontinuous_mode_test();

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
