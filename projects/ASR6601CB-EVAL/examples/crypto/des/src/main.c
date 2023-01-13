#include "tremo_rcc.h"
#include "drv.h"
#include "algorithm.h"
#include "des.h"

void des_ecb(void)
{
    U8 key[8] = {
        0x01,
        0x23,
        0x45,
        0x67,
        0x89,
        0xab,
        0xcd,
        0xef,
    };
    U8 in[8] = {
        0x5a,
        0x5a,
        0x5a,
        0x5a,
        0x5a,
        0x5a,
        0x5a,
        0x5a,
    };
    U8 cipher[8] = {
        0x72,
        0xaa,
        0xe3,
        0xb3,
        0xd6,
        0x91,
        0x6e,
        0x92,
    };
    U8 out[8];

    des_init(key, 0, NULL);
    des_crypto(in, 8, 0, out);
    des_close();
    if (!memcmp(out, cipher, 8)) {
        //OK
    } else {
        //ERROR
    }
}

int main(void)
{
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SAC, true);

    des_ecb();

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
