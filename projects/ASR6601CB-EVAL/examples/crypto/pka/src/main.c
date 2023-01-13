#include "tremo_rcc.h"
#include "drv.h"
#include "rng.h"
#include "algorithm.h"
#include "rsa.h"

U8 out[256];
U8 AData[32] = {
    // FFE0D0C0 B0A09080 70605040 302010FF
    0xFF,
    0x10,
    0x20,
    0x30,
    0x40,
    0x50,
    0x60,
    0x70,
    0x80,
    0x90,
    0xA0,
    0xB0,
    0xC0,
    0xD0,
    0xE0,
    0xFF,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
};

U8 BData[32] = {
    // F0EEDDCC BBAA9988 77665544 33221101
    0x01,
    0x11,
    0x22,
    0x33,
    0x44,
    0x55,
    0x66,
    0x77,
    0x88,
    0x99,
    0xAA,
    0xBB,
    0xCC,
    0xDD,
    0xEE,
    0xF0,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
};

U8 AaddB[32] = {
    //(½øÎ»01) F0CFAE8D 6C4B2A08 E7C6A584 63422200
    0x00,
    0x22,
    0x42,
    0x63,
    0x84,
    0xA5,
    0xC6,
    0xE7,
    0x08,
    0x2A,
    0x4B,
    0x6C,
    0x8D,
    0xAE,
    0xCF,
    0xF0,
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
};

U8 pka_cmp(U8* out, U8* cipher, U16 inLen)
{
    if (memcmp(out, cipher, inLen)) {
        return 1;
    } else {
        return 0;
    }
}

void pka_base_alg(U8* A, U8* B, U8* P, U8* out, U8* Expect, U16 Cmd, U32 Len)
{
    U32 ByteLen;
    U32 Len1;
    U8 ret;

    rsa_init();

    Len1    = Len;
    ByteLen = Len1 >> 3;

    if (Cmd == MODINV) {
        memcpy(RSA_HADR, A, ByteLen);
    } else {
        memcpy(RSA_AADR, A, ByteLen);
    }

    if (Cmd != LONGLSHIFT || Cmd != LONGRSHIFT || Cmd != MODINV) {
        memcpy(RSA_BADR, B, ByteLen);
    }

    if (P != NULL) {
        memcpy(RSA_PADR, P, ByteLen);
    }

    if (Cmd == LONGCMP) {
        ++Len;
    }
    Len1     = Len;
    PKAPLENL = Len;
    PKAPLENH = Len1 >> 8;

    PKACR = PKA_RUN | Cmd;
    while (PKACR & PKA_RUN)
        ;

    if (Cmd == MODADD || Cmd == MODSUB || Cmd == MODPSUB) {
        memcpy(out, RSA_BADR, ByteLen);
    } else if (Cmd != LONGCMP && Cmd != MODINV) {
        memcpy(out, RSA_AADR, ByteLen);
    } else if (Cmd == LONGCMP) {
        if (PKASR == 0xa5)
            ;
        else if (PKASR == 0x5a)
            ;
        else if (PKASR == 0x2F)
            ;
        else {
            return;
        }
    } else {
        // P0=PKASR;
        if (PKASR == 0xa5) {
        } else {
            memcpy(out, RSA_DADR, ByteLen);
        }
    }
    /*pka_close();*/
    if (Cmd != 4) {
        ret = pka_cmp(out, Expect, ByteLen);
        if (ret == 0) {
            //OK
        } else {
            //ERROR
        }
    }
}

int main(void)
{
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SAC, true);

    pka_base_alg(AData, BData, NULL, out, AaddB, LONGADD, 256);

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
