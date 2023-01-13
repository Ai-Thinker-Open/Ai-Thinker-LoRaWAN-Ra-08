#ifndef __BOOTLOADER_H_
#define __BOOTLOADER_H_

#define CM4_IRQ_VECT_BASE           0xE000ED08
#define CM4_IRQ_EN                  0xE000E100
#define CM4_IRQ_CLR                 0xE000E180

#define FLASH_START_ADDR            0x08000000
#define FLASH_MAX_SIZE              0x40000 //256KB

#define APP_START_ADDR              0x0800D000 //OFF-64KB

#define UART_INDEX                  UART0
#define UART_GPIOX                  GPIOB
#define UART_GPIO_PIN_TX            GPIO_PIN_1
#define UART_GPIO_PIN_RX            GPIO_PIN_0

#define BOOT_MODE_REG_BIT           (1<<29)

#define BOOT_MODE_GPIOX             GPIOA
#define BOOT_MODE_GPIO_PIN          GPIO_PIN_11

#define BOOT_MODE_JUMP2APP          1
#define BOOT_MODE_NO_JUMP           0

#define BOOT_LOADER_VERSION "ASRBOOTLOADER-V0.1";
#define BT_VERSION_SEG __attribute__((section("bt_version_sec")))


#define FLASH_OP_BEGIN()    __disable_irq()
#define FLASH_OP_END()      __enable_irq()

#define BOOTLOADER_MAX_CMD_SIZE   (529)
#define BOOTLOADER_MIN_CMD_SIZE   (9)

#define BOOTLOADER_CMD_SYNC        1
#define BOOTLOADER_CMD_JUMP        2
#define BOOTLOADER_CMD_FLASH       3
#define BOOTLOADER_CMD_ERASE       4
#define BOOTLOADER_CMD_VERIFY      5
#define BOOTLOADER_CMD_WROTP       6
#define BOOTLOADER_CMD_RDOTP       7 
#define BOOTLOADER_CMD_WROPT0      8
#define BOOTLOADER_CMD_RDOPT0      9
#define BOOTLOADER_CMD_WROPT1      10
#define BOOTLOADER_CMD_RDOPT1      11
#define BOOTLOADER_CMD_REBOOT      12
#define BOOTLOADER_CMD_SN          13
#define BOOTLOADER_CMD_WRREG       14
#define BOOTLOADER_CMD_RDREG       15
#define BOOTLOADER_CMD_BAUDRATE    16
#define BOOTLOADER_CMD_VERSION     17


#define BOOTLOADER_SYMBOL_CMD_START 0xFE
#define BOOTLOADER_SYMBOL_CMD_END   0xEF

#define BOOTLOADER_POS_START       0
#define BOOTLOADER_POS_CMD         1
#define BOOTLOADER_POS_STATUS      1
#define BOOTLOADER_POS_LEN_LSB     2
#define BOOTLOADER_POS_LEN_MSB     3
#define BOOTLOADER_POS_DATA        4

#define BOOTLOADER_STATUS_OK            0
#define BOOTLOADER_STATUS_UNKNOWN_CMD   11
#define BOOTLOADER_STATUS_ERR_SIZE      12
#define BOOTLOADER_STATUS_ERR_DATA      13
#define BOOTLOADER_STATUS_ERR_CHECKSUM  14
#define BOOTLOADER_STATUS_ERR_PARAM     15
#define BOOTLOADER_STATUS_ERR_FLASH     16
#define BOOTLOADER_STATUS_ERR_VERIFY    17

#define RES_UNSENT                      0
#define RES_SENT                        1

typedef struct _loader_req{
    uint8_t cmd;
    uint8_t *data;
    uint16_t data_len;
}loader_req_t;

typedef struct _loader_res{
    uint8_t status;
    uint8_t *data;
    uint16_t data_len;
}loader_res_t;


void boot_to_app(uint32_t addr);
void boot_handle_cmd(void);

#endif //__BOOTLOADER_H_