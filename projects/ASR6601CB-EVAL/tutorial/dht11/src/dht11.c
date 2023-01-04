#include <stdio.h>
#include "dht11.h"
#include "tremo_gpio.h"

unsigned char uchar_flag;
unsigned char uchar_temp;
unsigned char uchar_com_data;

unsigned char ucharRH_data_H_temp;
unsigned char ucharRH_data_L_temp;
unsigned char ucharT_data_H_temp;
unsigned char ucharT_data_L_temp;
unsigned char ucharcheckdata_temp;

unsigned char ucharRH_data_H;
unsigned char ucharRH_data_L;
unsigned char ucharT_data_H;
unsigned char ucharT_data_L;
unsigned char ucharcheckdata;

float Humi, Temp;
uint8_t dht11_pin;

void output_low()
{
    gpio_write(GPIOA, dht11_pin, GPIO_LEVEL_LOW);
}

void output_high()
{
    gpio_write(GPIOA, dht11_pin, GPIO_LEVEL_HIGH);
}

void input_initial()
{
    gpio_set_iomux(GPIOA, dht11_pin, 0);
    // gpio_init(GPIOA, dht11_pin, GPIO_MODE_INPUT_FLOATING);
    // gpio_init(GPIOA,dht11_pin,GPIO_MODE_INPUT_PULL_UP);
    
    // gpio_config_interrupt(GPIOA, dht11_pin, GPIO_INTR_RISING_FALLING_EDGE);
}

gpio_level_t read_data()
{
    return gpio_read(GPIOA, dht11_pin);
}

void COM()
{
    unsigned char i;
    for (i = 0; i < 8; i++)
    {
        uchar_flag = 2;
        // 等待IO口变低，变低后，通过延时去判断是0还是1
        while ((read_data() != GPIO_LEVEL_HIGH) && uchar_flag++)
        {
            delay_us(10);
        }
        delay_us(35); // 延时35us
        uchar_temp = 0;
        // 如果这个位是1，35us后，还是1，否则为0
        if (read_data() == GPIO_LEVEL_HIGH)
        {
            uchar_temp = 1;
        }
        uchar_flag = 2;
        // 等待IO口变高，变高后，表示可以读取下一位
        while ((read_data() == GPIO_LEVEL_HIGH) && uchar_flag++)
        {
            delay_us(10);
        }
        if (uchar_flag == 1)
        {
            printf("COM first step over time\n");
            break;
        }
        uchar_com_data <<= 1;
        uchar_com_data |= uchar_temp;
    }
    printf("COM first step uchar_com_data = %x\n", uchar_com_data);
}

void dht11_setup(uint8_t gpio_pin)
{
    printf("读取程序开始\n");
    dht11_pin = gpio_pin;
    gpio_init(GPIOA,dht11_pin,GPIO_MODE_OUTPUT_PP_LOW);
    output_low();
    delay_ms(20);
    printf("主机拉低电平20ms\n");
    output_high();
    delay_us(30);
    printf("主机拉高电平30um\n");
    input_initial();
}

void dht11_reset(){
    printf("发送复位信号\n");
    //设置为输出低电平
    gpio_init(GPIOA,dht11_pin,GPIO_MODE_OUTPUT_PP_LOW);
    output_low();
    delay_ms(20);
    printf("主机拉低电平20ms\n");
    //设置为输出高电平
    output_high();
    delay_us(30);
    printf("主机拉高电平30um\n");
    //设置为输入模式
    input_initial();
}

void dht11_read()
{
    printf("读取DHT11响应\n");
    if (read_data() != GPIO_LEVEL_LOW)
    {
        printf("%s\n", "read data failed!");
        return;
    }    
    uchar_flag = 2;
    // 判断从机是否发出 80us 的低电平响应信号是否结束
    printf("现在是低电平，等待从机拉高--%d\n",read_data());
    while ((read_data() == GPIO_LEVEL_LOW) && uchar_flag++)
    {
        delay_us(10);
    }
    uchar_flag = 2;
    printf("现在是高电平，等待从机拉低--%d\n",read_data());
    // 判断从机是否发出 80us 的高电平，如发出则进入数据接收状态
    while ((read_data() == GPIO_LEVEL_HIGH) && uchar_flag++)
    {
        printf("wait for low %d\n",uchar_flag);
        delay_us(30);
    }
    COM(); // 读取第1字节，
    ucharRH_data_H_temp = uchar_com_data;
    printf("读取第一位：%x\n", uchar_com_data);
    COM(); // 读取第2字节，
    ucharRH_data_L_temp = uchar_com_data;
    COM(); // 读取第3字节，
    ucharT_data_H_temp = uchar_com_data;
    COM(); // 读取第4字节，
    ucharT_data_L_temp = uchar_com_data;
    COM(); // 读取第5字节，
    ucharcheckdata_temp = uchar_com_data;
    output_high();
    // 判断校验和是否一致
    uchar_temp = (ucharT_data_H_temp + ucharT_data_L_temp + ucharRH_data_H_temp + ucharRH_data_L_temp);
    if (uchar_temp == ucharcheckdata_temp)
    {
        // 校验和一致，
        ucharRH_data_H = ucharRH_data_H_temp;
        ucharRH_data_L = ucharRH_data_L_temp;
        ucharT_data_H = ucharT_data_H_temp;
        ucharT_data_L = ucharT_data_L_temp;
        ucharcheckdata = ucharcheckdata_temp;
        // 保存温度和湿度
        Humi = ucharRH_data_H;
        Humi = ((unsigned short)Humi << 8 | ucharRH_data_L) / 10;

        Temp = ucharT_data_H;
        Temp = ((unsigned short)Temp << 8 | ucharT_data_L) / 10;
        printf("1=%f,2=%f\n", Humi, Temp);
    }
}
