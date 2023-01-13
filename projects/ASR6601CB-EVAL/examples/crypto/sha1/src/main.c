#include "tremo_rcc.h"
#include "sha1.h"
#include "algorithm.h"

static U8 tempbuf[0x200];

U8 data_cmp(U8* data, U8* data_exp, U16 inLen)
{
    if (memcmp(data, data_exp, inLen)) {
        return 1;
    } else {
        return 0;
    }
}

void sha1_test()
{
    U8 Result[20]  = { 0x72, 0x9A, 0x73, 0x15, 0x66, 0x09, 0x0C, 0x14, 0xB9, 0x61, 0xF6, 0xA1, 0x2B, 0xA2, 0x2A, 0x64,
        0xFD, 0xDF, 0xB4, 0x92 };
    U8 Result1[20] = { 0x54, 0x85, 0x38, 0x58, 0x2F, 0xD2, 0xE8, 0x77, 0xB0, 0x23, 0xBE, 0x06, 0x11, 0x15, 0x0D, 0xF9,
        0xE7, 0xCA, 0x99, 0xE8 };
    U8* InBuff     = tempbuf;
    U8* OutBuff    = tempbuf + 160;

    U32 i;

    memset(InBuff, 0, 160);
    memset(OutBuff, 0, 64);

    for (i = 0; i < 136; i++) {
        InBuff[i] = i;
    }
    sha1_hash(InBuff, 132, OutBuff);
    data_cmp(OutBuff, Result1, 20);

    for (i = 0; i < 136; i++) {
        InBuff[i] = 0x5a;
    }
    // case 1
    sha1_hash(InBuff, 132, OutBuff);
    data_cmp(OutBuff, Result, 20);
}

int main(void)
{
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SAC, true);

    sha1_test();

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
