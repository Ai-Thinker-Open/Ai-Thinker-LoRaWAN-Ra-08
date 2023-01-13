#include "tremo_rcc.h"
#include "rng.h"

void rng_test(void)
{
    UINT8 tmpBuf[0x100];

    rng_init(0x0f, 0xFF);
    rng_get_rand(tmpBuf, 0x10);
    rng_close();
}

int main(void)
{
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_RNGC, true);

    rng_test();

    /* Infinite loop */
    while (1) { }

    return 0;
}

#ifdef USE_FULL_ASSERT
void assert_failed(void* file, uint32_t line)
{
    (void)file;
    (void)line;

    while (1) { }
}
#endif
