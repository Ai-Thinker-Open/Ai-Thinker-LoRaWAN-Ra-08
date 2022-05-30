#include "tremo_lcd.h"
#include "tremo_rcc.h"
#include "system_cm4.h"


/**
  * @brief  LCD initialization
  * @param[in]  duty LCD duty configuration
  * @param[in]  bias LCD bias configuration
  * @param[in]  prescaler LCD prescaler configuration
  * @param[in]  division_clock LCD division clock configuration
  * @return None
  */
void lcd_init(lcd_duty_t duty, lcd_bias_t bias, lcd_prescaler_t prescaler, lcd_division_t division_clock)
{
    lcd_config_duty(duty);
    lcd_config_bias(bias);
    lcd_config_prescaler(prescaler);
    lcd_config_division_clock(division_clock);
}

/**
  * @brief  LCD enable
  * @param[in]  enable_flag enable or disable flag
  * @return None
  */
void lcd_enable(bool enable_flag)
{
	  while (lcd_check_sync_done() != true);
    if (enable_flag == true)
    {
        LCD -> CR0 |= LCD_ENABLE;
    }
		else
		{
        LCD -> CR0 &= (uint32_t)(~(uint32_t)LCD_ENABLE);
		}
}

/**
  * @brief  LCD config duty
  * @param[in]  duty LCD duty configuration
  * @return None
  */
void lcd_config_duty(lcd_duty_t duty)
{
    if ((duty != LCD_STATIC_DUTY) && (duty != LCD_1_2_DUTY) &&
        (duty != LCD_1_3_DUTY) && (duty != LCD_1_4_DUTY))
    {
        return;
    }
	  while (lcd_check_sync_done() != true);
    LCD -> CR0 &= (uint32_t)(~(uint32_t)LCD_RESV_DUTY);
	  while (lcd_check_sync_done() != true);
    LCD -> CR0 |= duty;
}

/**
  * @brief  LCD config bias
  * @param[in]  bias LCD bias configuration
  * @return None
  */
void lcd_config_bias(lcd_bias_t bias)
{
	  while (lcd_check_sync_done() != true);
    LCD -> CR0 &= (uint32_t)(~(uint32_t)LCD_STAITC_BIAS);
	  while (lcd_check_sync_done() != true);
    LCD -> CR0 |= bias;
}

/**
  * @brief  LCD config prescaler
  * @param[in]  prescaler LCD prescaler configuration
  * @return None
  */
void lcd_config_prescaler(lcd_prescaler_t prescaler)
{
	  while (lcd_check_sync_done() != true);
    LCD -> CR1 &= (uint32_t)(~(uint32_t)LCD_1_32768_PRESCALER);
	  while (lcd_check_sync_done() != true);
    LCD -> CR1 |= prescaler;
}

/**
  * @brief  LCD config division clock
  * @param[in]  division_clock LCD division clock configuration
  * @return None
  */
void lcd_config_division_clock(lcd_division_t division_clock)
{
	  while (lcd_check_sync_done() != true);
    LCD -> CR1 &= (uint32_t)(~(uint32_t)LCD_1_31_DIVISION);
	  while (lcd_check_sync_done() != true);
    LCD -> CR1 |= division_clock;
}

/**
  * @brief  LCD config blink frequence
  * @param[in]  blink_freq LCD blink frequence configuration
  * @return None
  */
void lcd_config_blink_freq(lcd_blink_freq_t blink_freq)
{
	  while (lcd_check_sync_done() != true);
    LCD -> CR1 &= (uint32_t)(~(uint32_t)LCD_1_1024_BLINK_FREQ);
	  while (lcd_check_sync_done() != true);
    LCD -> CR1 |= blink_freq;
}

/**
  * @brief  LCD config blink selection
  * @param[in]  blink_sel LCD blink selection configuration
  * @return None
  */
void lcd_config_blink_sel(lcd_blink_sel_t blink_sel)
{
	  while (lcd_check_sync_done() != true);
    LCD -> CR1 &= (uint32_t)(~(uint32_t)LCD_ALL_SEG_COM_BLINK);
	  while (lcd_check_sync_done() != true);
    LCD -> CR1 |= blink_sel;
}

/**
  * @brief  LCD config dead cycle
  * @param[in]  dead_cycle LCD dead cycle configuration
  * @return None
  */
void lcd_config_dead_cycle(lcd_dead_cycle_t dead_cycle)
{
	  while (lcd_check_sync_done() != true);
    LCD -> CR1 &= (uint32_t)(~(uint32_t)LCD_7_DEAD_CYCLE);
	  while (lcd_check_sync_done() != true);
    LCD -> CR1 |= dead_cycle;
}

/**
  * @brief  LCD enable switch
  * @param[in]  enable_flag enable or disable flag
  * @return None
  */
void lcd_enable_switch(bool enable_flag)
{
	  while (lcd_check_sync_done() != true);
    if (enable_flag == true)
    {
        LCD -> CR1 |= LCD_SWITCH_ENABLE;
    }
		else
		{
        LCD -> CR1 &= (uint32_t)(~(uint32_t)LCD_SWITCH_ENABLE);
		}
}

/**
  * @brief  LCD config large current number
  * @param[in]  large_current_num LCD large current number configuration
  * @return None
  */
void lcd_config_large_current_num(lcd_large_current_num_t large_current_num)
{
	  while (lcd_check_sync_done() != true);
    LCD -> CR1 &= (uint32_t)(~(uint32_t)LCD_7_LARGE_CURRENT_NUM);
	  while (lcd_check_sync_done() != true);
    LCD -> CR1 |= large_current_num;
}

/**
  * @brief  LCD write segment bit of COM
  * @param[in]  com_num LCD COM number
  * @param[in]  bit_position LCD segment bit of COM
  * @param[in]  value LCD segment bit value of COM
  * @return None
  */
void lcd_write_com_seg_bit(lcd_com_num_t com_num, uint8_t bit_position, uint8_t value)
{
    uint32_t bit_mask;
    uint32_t bit_value;

    bit_mask = 1 << bit_position;
    bit_value = value << bit_position;
	  while (lcd_check_sync_done() != true);
    if (com_num == LCD_COM_0)
    {
        LCD -> DR0 &= (uint32_t)(~(uint32_t)bit_mask);
	      while (lcd_check_sync_done() != true);
        LCD -> DR0 |= bit_value;
    }
		else if (com_num == LCD_COM_1)
    {
        LCD -> DR1 &= (uint32_t)(~(uint32_t)bit_mask);
	      while (lcd_check_sync_done() != true);
        LCD -> DR1 |= bit_value;
    }
		else if (com_num == LCD_COM_2)
    {
        LCD -> DR2 &= (uint32_t)(~(uint32_t)bit_mask);
	      while (lcd_check_sync_done() != true);
        LCD -> DR2 |= bit_value;
    }
		else if (com_num == LCD_COM_3)
    {
        LCD -> DR3 &= (uint32_t)(~(uint32_t)bit_mask);
	      while (lcd_check_sync_done() != true);
        LCD -> DR3 |= bit_value;
    }
}

/**
  * @brief  LCD clear segment of COM state
  * @param[in]  com_num LCD COM number
  * @return None
  */
void lcd_clear_com_seg_state(lcd_com_num_t com_num)
{
	  while (lcd_check_sync_done() != true);
    if (com_num == LCD_COM_0)
    {
        LCD -> DR0 &= (uint32_t)(~(uint32_t)LCD_COM0_SEG_MASK);
    }
		else if (com_num == LCD_COM_1)
    {
        LCD -> DR1 &= (uint32_t)(~(uint32_t)LCD_COM1_SEG_MASK);
    }
		else if (com_num == LCD_COM_2)
    {
        LCD -> DR2 &= (uint32_t)(~(uint32_t)LCD_COM2_SEG_MASK);
    }
		else if (com_num == LCD_COM_3)
    {
        LCD -> DR3 &= (uint32_t)(~(uint32_t)LCD_COM3_SEG_MASK);
    }
}

/**
  * @brief  LCD check synchronization done
  * @return bool
  */
bool lcd_check_sync_done()
{
    if (((LCD -> SR & LCD_CR0_DONE_STATUS) == LCD_CR0_DONE_STATUS) && ((LCD -> SR & LCD_CR1_DONE_STATUS) == LCD_CR1_DONE_STATUS))
    {
        return true;
    }
		else
    {
        return false;
    }
}

/**
  * @brief  LCD config drive mode
  * @param[in]  drive_mode LCD drive mode
  * @return None
  */
void lcd_config_drive_mode(lcd_drive_mode_t drive_mode)
{
    uint32_t readData;

    if ((drive_mode != LCD_SMALL_CURRENT_MODE) && (drive_mode != LCD_LARGE_CURRENT_MODE) &&
        (drive_mode != LCD_SMALL_CURRENT_BUFFER_MODE) && (drive_mode != LCD_LARGE_CURRENT_BUFFER_MODE))
    {
        return;
    }

    readData = TREMO_ANALOG_RD(0xb);
    readData = readData & (uint32_t)(~(uint32_t)(0x3 << 3));
    TREMO_ANALOG_WR(0xb, readData);
    readData = readData | (uint32_t)(drive_mode << 3);
    TREMO_ANALOG_WR(0xb, readData);
}

/**
  * @brief  LCD config interrupt
  * @param[in]  enable_flag ENABLE or DISABLE interrupt
  * @return None
  */
void lcd_config_interrupt(bool enable_flag)
{
    if (enable_flag == true)
    {
        LCD -> CR2 |= LCD_EVEN_FRAME_DONE_INT;
    }
		else
		{
        LCD -> CR2 &= (uint32_t)(~(uint32_t)LCD_EVEN_FRAME_DONE_INT);
		}
}

/**
  * @brief  LCD get interrupt status
  * @return bool
  */
bool lcd_get_interrupt_status()
{
    if ((LCD -> CR2 & LCD_EVEN_FRAME_DONE_INT_STATUS) == LCD_EVEN_FRAME_DONE_INT_STATUS)
    {
        return true;
    }
		else
    {
        return false;
    }
}

/**
  * @brief  LCD clear interrupt status
  * @return bool
  */
void lcd_clear_interrupt_status()
{
    LCD -> CR2 |= LCD_EVEN_FRAME_DONE_INT_STATUS;
}

/**
  * @brief  LCD enable COM
  * @param[in]  com_use LCD COM number is used
  * @return None
  */
void lcd_enable_com(lcd_com_use_t com_use)
{
    uint8_t com = (uint8_t)com_use;
    uint32_t readData;
    uint8_t i;

    if ((com_use != LCD_USE_1_COM) && (com_use != LCD_USE_2_COM) &&
        (com_use != LCD_USE_3_COM) && (com_use != LCD_USE_4_COM))
    {
        return;
    }

    readData = TREMO_ANALOG_RD(0x9);
    readData = readData & (uint32_t)(~(uint32_t)(0x7 << 1));
    TREMO_ANALOG_WR(0x9, readData);
    readData = readData | (uint32_t)((com - 1) << 1);
    TREMO_ANALOG_WR(0x9, readData);

    readData = TREMO_ANALOG_RD(0xb);
    for (i = 31; i > (31 - (com - 1)); i--)
    {
        readData = readData | (uint32_t)(0x1 << i);
    }
    TREMO_ANALOG_WR(0xb, readData);

    readData = TREMO_ANALOG_RD(0xa);
    readData = readData | (0x1 << 22);
    TREMO_ANALOG_WR(0xa, readData);
}

/**
  * @brief  LCD enable segment
  * @param[in]  seg_begin segment begin No
  * @param[in]  seg_num enabled segment number
  * @return None
  */
void lcd_enable_seg(uint8_t seg_begin, uint8_t seg_num)
{
    uint32_t readData;
    uint32_t bitSel;
    uint8_t i;

    bitSel = 1 << (seg_begin + 5);
    for (i = seg_begin + 1; i < (seg_begin + seg_num); i++)
    {
        bitSel |= (1 << (i + 5));
    }
    readData = TREMO_ANALOG_RD(0xb);
    readData = readData | bitSel;
    TREMO_ANALOG_WR(0xb, readData);
}

/**
  * @brief  LCD enable analog
  * @return None
  */
void lcd_enable_analog()
{
    uint32_t readData;

    readData = TREMO_ANALOG_RD(0x6);
    readData = readData & (uint32_t)(~(uint32_t)(0x1 << 7));
    TREMO_ANALOG_WR(0x6, readData);
}

