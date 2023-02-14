#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "delay.h"
#include "timer.h"
#include "radio.h"
#include "tremo_system.h"
#include "tremo_gpio.h"
#include "lora_net.h"
#include "lora_driver.h"

uint8_t SLAVE1_ADDR = 0x00; // 本地地址默认为0
uint8_t Addr_Num = 30;

#if defined(REGION_AS923)

#define RF_FREQUENCY 923000000 // Hz

#elif defined(REGION_AU915)

#define RF_FREQUENCY 915000000 // Hz

#elif defined(REGION_CN470)

#define RF_FREQUENCY 470000000 // Hz

#elif defined(REGION_CN779)

#define RF_FREQUENCY 779000000 // Hz

#elif defined(REGION_EU433)

#define RF_FREQUENCY 433000000 // Hz

#elif defined(REGION_EU868)

#define RF_FREQUENCY 868000000 // Hz

#elif defined(REGION_KR920)

#define RF_FREQUENCY 920000000 // Hz

#elif defined(REGION_IN865)

#define RF_FREQUENCY 865000000 // Hz

#elif defined(REGION_US915)

#define RF_FREQUENCY 915000000 // Hz

#elif defined(REGION_US915_HYBRID)

#define RF_FREQUENCY 915000000 // Hz

#else
#error "Please define a frequency band in the compiler options."
#endif

#define TX_OUTPUT_POWER 14 // dBm

#if defined(USE_MODEM_LORA)

#define LORA_BANDWIDTH 0        // [0: 125 kHz,
                                //  1: 250 kHz,
                                //  2: 500 kHz,
                                //  3: Reserved]
#define LORA_SPREADING_FACTOR 7 // [SF7..SF12]
#define LORA_CODINGRATE 1       // [1: 4/5,
                                //  2: 4/6,
                                //  3: 4/7,
                                //  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8  // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0   // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false

#elif defined(USE_MODEM_FSK)

#define FSK_FDEV 25000            // Hz
#define FSK_DATARATE 50000        // bps
#define FSK_BANDWIDTH 1000000     // Hz >> DSB in sx126x
#define FSK_AFC_BANDWIDTH 1000000 // Hz
#define FSK_PREAMBLE_LENGTH 5     // Same for Tx and Rx
#define FSK_FIX_LENGTH_PAYLOAD_ON false

#else
#error "Please define a modem in the compiler options."
#endif

typedef enum
{
    LORA_IDLE,
    RX,
    RX_TIMEOUT,
    RX_ERROR,
    TX,
    TX_TIMEOUT
} States_t;

#define RX_TIMEOUT_VALUE 1800
#define BUFFER_SIZE 256 // Define the payload size here

uint16_t BufferSize = BUFFER_SIZE;
uint8_t Buffer[BUFFER_SIZE];
char ACKBuf[] = "11111111111122222222222222222333333333333333333333334444444444444444444445555555555555555555555556666666666666666666666677777777777777777777777777777888888888888888888889999999999999990000000000000000"; // 发送的数据

volatile States_t State = LORA_IDLE;

int8_t RssiValue = 0;
int8_t SnrValue = 0;

uint32_t ChipId[2] = {0};
uint8_t sendMsgFlag = 2; // 0：空闲状态可以发送数�?�?1：�?�在发送，等待发送完成；2：发送完�?

/*!
 * Radio events function pointer
 */
static RadioEvents_t RadioEvents;

/*!
 * \brief Function to be executed on Radio Tx Done event
 */
void OnTxDone(void);

/*!
 * \brief Function to be executed on Radio Rx Done event
 */
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);

/*!
 * \brief Function executed on Radio Tx Timeout event
 */
void OnTxTimeout(void);

/*!
 * \brief Function executed on Radio Rx Timeout event
 */
void OnRxTimeout(void);

/*!
 * \brief Function executed on Radio Rx Error event
 */
void OnRxError(void);

/**
 * 功能：发送数�?�?
 * 参数�?
 *       buffer:数据包地址
 *       len:数据包长�?
 * 返回值：None
 */
void transmitPackets(unsigned char *sendBuf, unsigned char len)
{

    for (int i = 0; i < len; i++)
    {
        printf("[] transmitPackets Buffer %d-%d\r\n", i, sendBuf[i]);
    }

    printf("State: %d ,lenth %d ,Node send message \r\n", State, len);

#ifdef CONFIG_GATEWAY

#else
    delay_ms(500);
#endif
    Radio.Send((uint8_t *)sendBuf, len);
}

uint8_t rx_data[8] = {0};
uint8_t rx_index = 0;

int serial_output(uint8_t *buffer, int len)
{

    for (int i = 0; i < len; i++)
    {
        uart_send_data(UART0, buffer[i]);
    }
    return 0;
}

void handler_uart_data(uint8_t data)
{

    int isOK = 0;
    rx_data[rx_index++] = data;
    // A0 F1 C0 03 11 22 28 A1
    if (rx_index > 5 && rx_data[0] == 0xA0 && rx_data[rx_index - 1] == 0xA1)
    {
#ifdef CONFIG_GATEWAY
        // rx_data[rx_index - 1] = '\0';
        // printf("%s", rx_data);

        uint8_t checkData = 0;

        for (int i = 0; i < rx_index; i++)
        {
            checkData = rx_data[i] + checkData;
        }

        checkData = checkData - rx_data[rx_index - 2];
        if (checkData == rx_data[rx_index - 2])
        {
            // printf("OK Slave:%d , data:%d \n", rx_data[2], rx_data[3]);
            if (rx_data[1] == 0xF2)
            {
                Addr_Num = rx_data[2];
                printf("set polling device num: %d\r\n", Addr_Num);
            }
            else if (rx_data[1] == 0xF1)
            {
                Radio.Send(rx_data, rx_index);
            }
            else
            {
                printf("command ERROR");
            }
            isOK = 1;
            rx_data[0] = '\0';
            rx_index = 0;
        }
        else
        {

            for (int i = 0; i < rx_index; i++)
            {
                printf("%02X ", rx_data[i]);
            }

            printf("-%02X ", checkData);
            rx_data[0] = '\0';
            rx_index = 0;
        }
#else
        SLAVE1_ADDR = rx_data[1];
        printf("set Node addr: %d\r\n", SLAVE1_ADDR);
#endif
    }
    else if (rx_data[0] != 0xA0)
    {
        rx_data[0] = '\0';
        rx_index = 0;
    }

    if (rx_index > 15)
    {
        rx_data[0] = '\0';
        rx_index = 0;
    }

    if (isOK)
    {
        gpio_write(GPIOA, GPIO_PIN_4, GPIO_LEVEL_LOW);
        gpio_write(GPIOA, GPIO_PIN_5, GPIO_LEVEL_LOW);
        gpio_write(GPIOA, GPIO_PIN_7, GPIO_LEVEL_HIGH);
    }
    else
    {
        gpio_write(GPIOA, GPIO_PIN_4, GPIO_LEVEL_LOW);
        gpio_write(GPIOA, GPIO_PIN_5, GPIO_LEVEL_HIGH);
        gpio_write(GPIOA, GPIO_PIN_7, GPIO_LEVEL_LOW);
    }
}

int Ra08KitLoraTestStart(void)
{
    static uint8_t ledStatus = 0;
    uint8_t send_buff[8] = {0xA0, 0xF1, 0x01, 0x01, 0x11, 0x22, 0x33, 0xA1};
    DeviceSta_Strcture device = {0};
    DeviceBlock DeviceBlock_Structure;
    DeviceBlock DeviceBlock_StructureArray[2];
    int i = 0;

    printf("Ra-08-kit test Start! ");

    (void)system_get_chip_id(ChipId);

    // mode：工作模式，LoRa�?片可工作在FSK模式/LoRa模式�?
    // power：LoRa工作功率
    // fdev：FSK模式下�?�置，不考虑使用
    // bandwidth：带�?-125K�?250K�?500K
    // datarate：扩频因�?
    // coderate�? 编码�?
    // preambleLen：前导码长度
    // fixLen：数�?包长度是否固�?
    // crcOn：是否启动CRC校验
    // freqHopOn：调频使能开�?
    // hopPeriod：若�?动调频，调�?�周期确�?
    // iqInverted：反转IQ信号(笔者也没搞过呀)
    // timeout：�?�置超时时间（一�?�?接收超时时间�?

    // Radio initialization
    RadioEvents.TxDone = OnTxDone;
    RadioEvents.RxDone = OnRxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxTimeout = OnRxTimeout;
    RadioEvents.RxError = OnRxError;

    Radio.Init(&RadioEvents);

    // 设置LoRa�?片工作�?�率 #define RF_FREQUENCY 470000000 // Hz 953525
    Radio.SetChannel(433953525);

#if defined(USE_MODEM_LORA)

    printf("Ra-08-kit USE_MODEM_LORA \r\n");

    Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                      LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                      LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                      true, 0, 0, LORA_IQ_INVERSION_ON, 3000);

    Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                      LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                      LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                      0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

    printf("freq: %lu\r\n,Lora Mode\r\nversion:0.0.0\r\ntx power:%d\r\nSF:%d\r\nCR:4/%d\r\n", RF_FREQUENCY, TX_OUTPUT_POWER, LORA_SPREADING_FACTOR, LORA_CODINGRATE + 4);

    switch (LORA_BANDWIDTH)
    {
    case 0:
        printf("BW:125kHz\r\n");
        break;
    case 1:
        printf("BW:250kHz\r\n");
        break;
    case 2:
        printf("BW:500kHz\r\n");
        break;
    default:
        printf("BW:Unknown:%d\r\n", LORA_BANDWIDTH);
        break;
    }
#elif defined(USE_MODEM_FSK)
    printf("Ra-08-kit USE_MODEM_FSK \r\n");

    Radio.SetTxConfig(MODEM_FSK, TX_OUTPUT_POWER, FSK_FDEV, 0,
                      FSK_DATARATE, 0,
                      FSK_PREAMBLE_LENGTH, FSK_FIX_LENGTH_PAYLOAD_ON,
                      true, 0, 0, 0, 3000);

    Radio.SetRxConfig(MODEM_FSK, FSK_BANDWIDTH, FSK_DATARATE,
                      0, FSK_AFC_BANDWIDTH, FSK_PREAMBLE_LENGTH,
                      0, FSK_FIX_LENGTH_PAYLOAD_ON, 0, true,
                      0, 0, false, true);

#else
#error "Please define a frequency band in the compiler options."
#endif

    Radio.Rx(RX_TIMEOUT_VALUE);

    while (1)
    {

#ifdef CONFIG_GATEWAY
        static bool flag = 0;
        static uint8_t node_addr = 0x01;
        static long count = 0;
        if (0 == flag)
        {
            // printf("GATEWAY ...\r\n");
            Radio.Send(send_buff, sizeof(send_buff));
            printf("GATEWAY send data:\r\n");
            for (int i = 0; i < sizeof(send_buff); i++)
            {
                printf("%02X ", send_buff[i]);
            }
            printf("\r\n");
            printf("flag: %d count: %d\r\n", flag, count);
            flag = 1;
        }
        count++;
        if (count > 1000000)
        {
            count = 0;
            flag = 0;
            printf("ADDR %d get ack timeout\r\n", node_addr);
            node_addr++;
            if (node_addr > Addr_Num)
            {
                if (send_buff[3] == 1)
                {
                    send_buff[3] = 2;
                }
                else
                {
                    send_buff[3] = 1;
                }
                node_addr = 0x01;
            }
            send_buff[2] = node_addr;
        }
#endif

        switch (State)
        {
        case RX:
            Radio.Rx(RX_TIMEOUT_VALUE);
            State = LORA_IDLE;

#ifdef CONFIG_GATEWAY
            if (Buffer[0] == 0xFF)
            {
                node_addr = Buffer[1] + 1;
                if (node_addr > Addr_Num)
                {
                    node_addr = 0x01;
                    if (send_buff[3] == 1)
                    {
                        send_buff[3] = 2;
                    }
                    else
                    {
                        send_buff[3] = 1;
                    }
                }
                send_buff[2] = node_addr;
                printf("Get data from: %d ,set next call node: %d, polling nodes num: %d \r\n", Buffer[1], node_addr, Addr_Num);
                printf("data: %s\r\n", (char *)(Buffer + 2));
                count = 0;
                flag = 0;
            }
            // sendMasterAsk(SLAVE1_ADDR, OP_R_SENSOR, PRAM_R_ALL); //主机发送指�?
            // printf("[%s()-%d] CONFIG_GATEWAY send message:%s\r\n", __func__, __LINE__, sendBuf);
            // printf("GATEWAY ...\r\n");
#else
            {

                // printf("Recieve New Msg , length:[%d] \r\n", BufferSize);
                // printf("Recieve SLAVE1_ADDR: %d, receive addr: %d\r\n", SLAVE1_ADDR, Buffer[2]);

                if (Buffer[1] == 0xFF)
                {
                    // 设置�?的�?�色
                    ledStatus = Buffer[3];
                    switch (ledStatus)
                    {
                    case 1:
                        gpio_write(GPIOA, GPIO_PIN_4, GPIO_LEVEL_HIGH);
                        gpio_write(GPIOA, GPIO_PIN_5, GPIO_LEVEL_LOW);
                        gpio_write(GPIOA, GPIO_PIN_7, GPIO_LEVEL_LOW);
                        break;
                    case 2:
                        gpio_write(GPIOA, GPIO_PIN_4, GPIO_LEVEL_LOW);
                        gpio_write(GPIOA, GPIO_PIN_5, GPIO_LEVEL_HIGH);
                        gpio_write(GPIOA, GPIO_PIN_7, GPIO_LEVEL_LOW);
                        break;
                    case 3:
                        gpio_write(GPIOA, GPIO_PIN_4, GPIO_LEVEL_LOW);
                        gpio_write(GPIOA, GPIO_PIN_5, GPIO_LEVEL_LOW);
                        gpio_write(GPIOA, GPIO_PIN_7, GPIO_LEVEL_HIGH);
                        break;
                    case 4:
                        gpio_write(GPIOA, GPIO_PIN_4, GPIO_LEVEL_LOW);
                        gpio_write(GPIOA, GPIO_PIN_5, GPIO_LEVEL_LOW);
                        gpio_write(GPIOA, GPIO_PIN_7, GPIO_LEVEL_LOW);
                        break;

                    case 5:
                        gpio_write(GPIOA, GPIO_PIN_4, GPIO_LEVEL_HIGH);
                        gpio_write(GPIOA, GPIO_PIN_5, GPIO_LEVEL_HIGH);
                        gpio_write(GPIOA, GPIO_PIN_7, GPIO_LEVEL_HIGH);
                        break;

                    default:
                        break;
                    }
                }
                else // 判断�?否为控制�?己？
                {
                    if (Buffer[2] == SLAVE1_ADDR)
                    {
                        printf("Recieve New Msg , length:[%d] \r\n", BufferSize);
                        printf("Recieve SLAVE1_ADDR: %d, receive addr: %d data: \r\n", SLAVE1_ADDR, Buffer[2]);
                        for (int i = 0; i < BufferSize; i++)
                        {
                            printf("%02X ", Buffer[i]);
                        }
                        printf("\r\n");
                        uint8_t data[205] = {0};
                        data[0] = 0xFF;
                        data[1] = SLAVE1_ADDR;
                        memmove(data + 2, ACKBuf, sizeof(ACKBuf));
                        Radio.Send(data, sizeof(ACKBuf));
                        // 设置�?的�?�色
                        if (Buffer[1] == 0xF1)
                        {
                            ledStatus = Buffer[3];
                            switch (ledStatus)
                            {
                            case 1:
                                gpio_write(GPIOA, GPIO_PIN_4, GPIO_LEVEL_HIGH);
                                gpio_write(GPIOA, GPIO_PIN_5, GPIO_LEVEL_LOW);
                                gpio_write(GPIOA, GPIO_PIN_7, GPIO_LEVEL_LOW);
                                break;
                            case 2:
                                gpio_write(GPIOA, GPIO_PIN_4, GPIO_LEVEL_LOW);
                                gpio_write(GPIOA, GPIO_PIN_5, GPIO_LEVEL_HIGH);
                                gpio_write(GPIOA, GPIO_PIN_7, GPIO_LEVEL_LOW);
                                break;
                            case 3:
                                gpio_write(GPIOA, GPIO_PIN_4, GPIO_LEVEL_LOW);
                                gpio_write(GPIOA, GPIO_PIN_5, GPIO_LEVEL_LOW);
                                gpio_write(GPIOA, GPIO_PIN_7, GPIO_LEVEL_HIGH);
                                break;

                            case 4:
                                gpio_write(GPIOA, GPIO_PIN_4, GPIO_LEVEL_LOW);
                                gpio_write(GPIOA, GPIO_PIN_5, GPIO_LEVEL_LOW);
                                gpio_write(GPIOA, GPIO_PIN_7, GPIO_LEVEL_LOW);
                                break;

                            case 5:
                                gpio_write(GPIOA, GPIO_PIN_4, GPIO_LEVEL_HIGH);
                                gpio_write(GPIOA, GPIO_PIN_5, GPIO_LEVEL_HIGH);
                                gpio_write(GPIOA, GPIO_PIN_7, GPIO_LEVEL_HIGH);
                                break;
                            default:
                                break;
                            }
                        }
                        // 读取�?的状�?
                        else if (Buffer[1] == 0xF2)
                        {
                            uint8_t data[] = {
                                0xF3,
                                SLAVE1_ADDR,
                                ledStatus,
                                0xF4};
                            Radio.Send(data, 4);
                        }
                    }
                    // else
                    // printf("[] Node is not me , error\r\n");
                }
            }
#endif

            break;
        case TX:
            // printf("[%s()-%d]Tx done\r\n", __func__, __LINE__);
            Radio.Rx(RX_TIMEOUT_VALUE);
            sendMsgFlag = 2;
            State = LORA_IDLE;
            break;
        case RX_TIMEOUT:
        case RX_ERROR:
            // printf("[%s()-%d]Rx timeout/error\r\n",__func__,__LINE__);
            Radio.Rx(RX_TIMEOUT_VALUE);
            State = LORA_IDLE;
            break;
        case TX_TIMEOUT:
            printf("[%s()-%d]Tx timeout\r\n", __func__, __LINE__);
            Radio.Rx(RX_TIMEOUT_VALUE);
            sendMsgFlag = 2;
            State = LORA_IDLE;
            break;
        case LORA_IDLE:
            if (0 == sendMsgFlag)
            {
                // Radio.Send((uint8_t *)sendBuf, strlen(sendBuf) + 1);
                sendMsgFlag = 1;
                // #ifdef CONFIG_GATEWAY
                //                 {
                //                     sendMasterAsk(SLAVE1_ADDR, OP_R_SENSOR, PRAM_R_ALL); //主机发送指�?
                //                     printf("[%s()-%d] CONFIG_GATEWAY send message:%s\r\n", __func__, __LINE__, sendBuf);
                //                 }
                // #else
                //                 {
                //                     // sendMasterAsk(SLAVE1_ADDR, OP_R_SENSOR, PRAM_R_ALL); //主机发送指�?
                //                     // printf("[%s()-%d] CONFIG_GATEWAY send message:%s\r\n", __func__, __LINE__, sendBuf);
                //                     printf("[%s()-%d] Node do not send message:%s \r\n", __func__, __LINE__);
                //                 }
                // #endif
            }
            break;
        default:
            printf("[%s()-%d]unknown case:%d\r\n", __func__, __LINE__, State);
            break;
        }

        // Process Radio IRQ
        Radio.IrqProcess();
    }
}

void OnTxDone(void)
{
    Radio.Sleep();
    State = TX;
}

/**
 * 功能：接收数�?�?
 * 参数�?
 *       buffer:数据包存放地址
 * 返回值：
 * 		 如果成功接收返回1，否则返�?0
 */
unsigned char receivePackets(unsigned char *buffer)
{
    if (BufferSize)
    {
        memset(buffer, 0, BUFFER_SIZE);
        memcpy(buffer, Buffer, BufferSize);
        return 1;
    }
    else
        return 0;
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
    Radio.Sleep();
    BufferSize = size;
    memset(Buffer, 0, BUFFER_SIZE);
    memcpy(Buffer, payload, BufferSize);
    RssiValue = rssi;
    SnrValue = snr;
    State = RX;
}

void OnTxTimeout(void)
{
    Radio.Sleep();
    State = TX_TIMEOUT;
}

void OnRxTimeout(void)
{
    Radio.Sleep();
    State = RX_TIMEOUT;
}

void OnRxError(void)
{
    Radio.Sleep();
    State = RX_ERROR;
}
