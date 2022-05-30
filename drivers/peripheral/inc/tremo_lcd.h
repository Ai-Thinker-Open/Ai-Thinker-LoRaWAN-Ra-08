/**
 ******************************************************************************
 * @file    tremo_lcd.h
 * @author  ASR Tremo Team
 * @version v1.5.0
 * @date    2021-06-23
 * @brief   Header file of LCD module.
 * @addtogroup Tremo_Drivers
 * @{
 * @defgroup LCD
 * @{
 */

#ifndef __TREMO_LCD_H
#define __TREMO_LCD_H

#ifdef __cplusplus
extern "C"{
#endif

#include "tremo_regs.h"
#include "stdbool.h"

	
/**
 * @brief LCD duty definition
 */
typedef enum {
    LCD_STATIC_DUTY = 0x0, /*!< LCD static duty*/
    LCD_1_2_DUTY = 0x1,    /*!< LCD 1/2 duty*/
    LCD_1_3_DUTY = 0x2,    /*!< LCD 1/3 duty*/
    LCD_1_4_DUTY = 0x3,    /*!< LCD 1/4 duty*/
    LCD_RESV_DUTY = 0x7    /*!< LCD reserved duty*/
} lcd_duty_t;

/**
 * @brief LCD COM total used definition
 */
typedef enum {
    LCD_USE_1_COM = 0x1, /*!< LCD 1 COM is used*/
    LCD_USE_2_COM = 0x2, /*!< LCD 2 COM is used*/
    LCD_USE_3_COM = 0x3, /*!< LCD 3 COM is used*/
    LCD_USE_4_COM = 0x4, /*!< LCD 4 COM is used*/
} lcd_com_use_t;

/**
 * @brief LCD drive mode definition
 */
typedef enum {
    LCD_SMALL_CURRENT_MODE = 0x0,        /*!< LCD small current mode*/
    LCD_LARGE_CURRENT_MODE = 0x1,        /*!< LCD large current mode*/
    LCD_SMALL_CURRENT_BUFFER_MODE = 0x2, /*!< LCD small current buffer mode*/
    LCD_LARGE_CURRENT_BUFFER_MODE = 0x3  /*!< LCD large current buffer mode*/
} lcd_drive_mode_t;

/**
 * @brief LCD bias definition
 */
typedef enum {
    LCD_1_4_BIAS = 0x0,    /*!< LCD 1/4 bias*/
    LCD_1_3_BIAS = 0x8,    /*!< LCD 1/3 bias*/
    LCD_1_2_BIAS = 0x10,   /*!< LCD 1/2 bias*/
    LCD_STAITC_BIAS = 0x18 /*!< LCD static bias*/
} lcd_bias_t;

/**
 * @brief LCD enable definition
 */
typedef enum {
    LCD_ENABLE = 0x20 /*!< LCD enable*/
} lcd_enable_t;

/**
 * @brief LCD prescaler definition
 */
typedef enum {
    LCD_0_PRESCALER = 0x0,       /*!< LCD prescaler clock is lcd clock*/
    LCD_1_2_PRESCALER = 0x1,     /*!< LCD prescaler clock is lcd clock/2*/
    LCD_1_4_PRESCALER = 0x2,     /*!< LCD prescaler clock is lcd clock/4*/
    LCD_1_8_PRESCALER = 0x3,     /*!< LCD prescaler clock is lcd clock/8*/
    LCD_1_16_PRESCALER = 0x4,    /*!< LCD prescaler clock is lcd clock/16*/
    LCD_1_32_PRESCALER = 0x5,    /*!< LCD prescaler clock is lcd clock/32*/
    LCD_1_64_PRESCALER = 0x6,    /*!< LCD prescaler clock is lcd clock/64*/
    LCD_1_128_PRESCALER = 0x7,   /*!< LCD prescaler clock is lcd clock/128*/
    LCD_1_256_PRESCALER = 0x8,   /*!< LCD prescaler clock is lcd clock/256*/
    LCD_1_512_PRESCALER = 0x9,   /*!< LCD prescaler clock is lcd clock/512*/
    LCD_1_1024_PRESCALER = 0xa,  /*!< LCD prescaler clock is lcd clock/1024*/
    LCD_1_2048_PRESCALER = 0xb,  /*!< LCD prescaler clock is lcd clock/2048*/
    LCD_1_4096_PRESCALER = 0xc,  /*!< LCD prescaler clock is lcd clock/4096*/
    LCD_1_8192_PRESCALER = 0xd,  /*!< LCD prescaler clock is lcd clock/8192*/
    LCD_1_16384_PRESCALER = 0xe, /*!< LCD prescaler clock is lcd clock/16384*/
    LCD_1_32768_PRESCALER = 0xf  /*!< LCD prescaler clock is lcd clock/32768*/
} lcd_prescaler_t;

/**
 * @brief LCD clock division definition
 */
typedef enum {
    LCD_1_16_DIVISION = 0x0,  /*!< LCD division clock is prescaler clock/16*/
    LCD_1_17_DIVISION = 0x10, /*!< LCD division clock is prescaler clock/17*/
    LCD_1_18_DIVISION = 0x20, /*!< LCD division clock is prescaler clock/18*/
    LCD_1_19_DIVISION = 0x30, /*!< LCD division clock is prescaler clock/19*/
    LCD_1_20_DIVISION = 0x40, /*!< LCD division clock is prescaler clock/20*/
    LCD_1_21_DIVISION = 0x50, /*!< LCD division clock is prescaler clock/21*/
    LCD_1_22_DIVISION = 0x60, /*!< LCD division clock is prescaler clock/22*/
    LCD_1_23_DIVISION = 0x70, /*!< LCD division clock is prescaler clock/23*/
    LCD_1_24_DIVISION = 0x80, /*!< LCD division clock is prescaler clock/24*/
    LCD_1_25_DIVISION = 0x90, /*!< LCD division clock is prescaler clock/25*/
    LCD_1_26_DIVISION = 0xa0, /*!< LCD division clock is prescaler clock/26*/
    LCD_1_27_DIVISION = 0xb0, /*!< LCD division clock is prescaler clock/27*/
    LCD_1_28_DIVISION = 0xc0, /*!< LCD division clock is prescaler clock/28*/
    LCD_1_29_DIVISION = 0xd0, /*!< LCD division clock is prescaler clock/29*/
    LCD_1_30_DIVISION = 0xe0, /*!< LCD division clock is prescaler clock/30*/
    LCD_1_31_DIVISION = 0xf0  /*!< LCD division clock is prescaler clock/31*/
} lcd_division_t;

/**
 * @brief LCD blink frequence definition
 */
typedef enum {
    LCD_1_8_BLINK_FREQ = 0x0,     /*!< LCD blink frequence is division clock/8*/
    LCD_1_16_BLINK_FREQ = 0x100,  /*!< LCD blink frequence is division clock/16*/
    LCD_1_32_BLINK_FREQ = 0x200,  /*!< LCD blink frequence is division clock/32*/
    LCD_1_64_BLINK_FREQ = 0x300,  /*!< LCD blink frequence is division clock/64*/
    LCD_1_128_BLINK_FREQ = 0x400, /*!< LCD blink frequence is division clock/128*/
    LCD_1_256_BLINK_FREQ = 0x500, /*!< LCD blink frequence is division clock/256*/
    LCD_1_512_BLINK_FREQ = 0x600, /*!< LCD blink frequence is division clock/512*/
    LCD_1_1024_BLINK_FREQ = 0x700 /*!< LCD blink frequence is division clock/1024*/
} lcd_blink_freq_t;

/**
 * @brief LCD blink selection definition
 */
typedef enum {
    LCD_DISABLE_BLINK = 0x0,         /*!< LCD blink disable*/
    LCD_SEG0_COM0_BLINK = 0x800,     /*!< LCD seg0 and com0 one pixel blink*/
    LCD_SEG0_ALL_COM_BLINK = 0x1000, /*!< LCD seg0 and all com eight pixels blink*/
    LCD_ALL_SEG_COM_BLINK = 0x1800   /*!< LCD seg and com all pixels blink*/
} lcd_blink_sel_t;

/**
 * @brief LCD dead cycle definition
 */
typedef enum {
    LCD_0_DEAD_CYCLE = 0x0,    /*!< LCD dead cycle is 0 division clock*/
    LCD_1_DEAD_CYCLE = 0x2000, /*!< LCD dead cycle is 1 division clock*/
    LCD_2_DEAD_CYCLE = 0x4000, /*!< LCD dead cycle is 2 division clock*/
    LCD_3_DEAD_CYCLE = 0x6000, /*!< LCD dead cycle is 3 division clock*/
    LCD_4_DEAD_CYCLE = 0x8000, /*!< LCD dead cycle is 4 division clock*/
    LCD_5_DEAD_CYCLE = 0xa000, /*!< LCD dead cycle is 5 division clock*/
    LCD_6_DEAD_CYCLE = 0xc000, /*!< LCD dead cycle is 6 division clock*/
    LCD_7_DEAD_CYCLE = 0xe000  /*!< LCD dead cycle is 7 division clock*/
} lcd_dead_cycle_t;

/**
 * @brief LCD enable switch definition
 */
typedef enum {
    LCD_SWITCH_ENABLE = 0x10000 /*!< LCD switch enable*/
} lcd_switch_enable_t;

/**
 * @brief LCD large current number definition
 */
typedef enum {
    LCD_0_LARGE_CURRENT_NUM = 0x0,     /*!< LCD large current number is 0 prescaler clock*/
    LCD_1_LARGE_CURRENT_NUM = 0x20000, /*!< LCD large current number is 1 prescaler clock*/
    LCD_2_LARGE_CURRENT_NUM = 0x40000, /*!< LCD large current number is 2 prescaler clock*/
    LCD_3_LARGE_CURRENT_NUM = 0x60000, /*!< LCD large current number is 3 prescaler clock*/
    LCD_4_LARGE_CURRENT_NUM = 0x80000, /*!< LCD large current number is 4 prescaler clock*/
    LCD_5_LARGE_CURRENT_NUM = 0xa0000, /*!< LCD large current number is 5 prescaler clock*/
    LCD_6_LARGE_CURRENT_NUM = 0xc0000, /*!< LCD large current number is 6 prescaler clock*/
    LCD_7_LARGE_CURRENT_NUM = 0xe0000  /*!< LCD large current number is 7 prescaler clock*/
} lcd_large_current_num_t;

/**
 * @brief LCD COM number definition
 */
typedef enum {
    LCD_COM_0 = 0, /*!< LCD COM0*/
    LCD_COM_1,     /*!< LCD COM1*/
    LCD_COM_2,     /*!< LCD COM2*/
    LCD_COM_3,     /*!< LCD COM3*/
} lcd_com_num_t;

/**
 * @brief LCD COM0 segment definition
 */
typedef enum {
    LCD_COM0_SEG_MASK = 0x7ffffff /*!< LCD COM0 segment mask*/
} lcd_com0_seg_mask_t;

/**
 * @brief LCD COM1 segment definition
 */
typedef enum {
    LCD_COM1_SEG_MASK = 0x3ffffff /*!< LCD COM1 segment mask*/
} lcd_com1_seg_mask_t;

/**
 * @brief LCD COM2 segment definition
 */
typedef enum {
    LCD_COM2_SEG_MASK = 0x1ffffff /*!< LCD COM2 segment mask*/
} lcd_com2_seg_mask_t;

/**
 * @brief LCD COM3 segment definition
 */
typedef enum {
    LCD_COM3_SEG_MASK = 0xffffff /*!< LCD COM3 segment mask*/
} lcd_com3_seg_mask_t;

/**
 * @brief LCD controller status definition
 */
typedef enum {
    LCD_CR0_DONE_STATUS = 0x1, /*!< LCD write cr0 done status*/
    LCD_CR1_DONE_STATUS = 0x2  /*!< LCD write cr1 done status*/
} lcd_status_t;

/**
 * @brief LCD config interrupt definition
 */
typedef enum {
    LCD_EVEN_FRAME_DONE_INT = 0x2 /*!< LCD even frame done interrupt*/
} lcd_interrupt_t;

/**
 * @brief LCD interrupt status definition
 */
typedef enum {
    LCD_EVEN_FRAME_DONE_INT_STATUS = 0x1 /*!< LCD even frame done interrupt status*/
} lcd_interrupt_status_t;



void lcd_init(lcd_duty_t duty, lcd_bias_t bias, lcd_prescaler_t prescaler, lcd_division_t division_clock);

void lcd_enable(bool enable_flag);

void lcd_config_duty(lcd_duty_t duty);
void lcd_config_bias(lcd_bias_t bias);
void lcd_config_prescaler(lcd_prescaler_t prescaler);
void lcd_config_division_clock(lcd_division_t division_clock);

void lcd_config_blink_freq(lcd_blink_freq_t blink_freq);
void lcd_config_blink_sel(lcd_blink_sel_t blink_sel);

void lcd_config_dead_cycle(lcd_dead_cycle_t dead_cycle);

void lcd_enable_switch(bool enable_flag);
void lcd_config_large_current_num(lcd_large_current_num_t large_current_num);

void lcd_write_com_seg_bit(lcd_com_num_t com_num, uint8_t bit_position, uint8_t value);
void lcd_clear_com_seg_state(lcd_com_num_t com_num);

bool lcd_check_sync_done();

void lcd_config_drive_mode(lcd_drive_mode_t drive_mode);

void lcd_config_interrupt(bool enable_flag);
bool lcd_get_interrupt_status();
void lcd_clear_interrupt_status();

void lcd_enable_com(lcd_com_use_t com_number);
void lcd_enable_seg(uint8_t seg_begin, uint8_t seg_num);

void lcd_enable_analog();

#ifdef __cplusplus
}
#endif
#endif /* __TREMO_LCD_H */

/**
 * @} 
 * @}
 */
