/*!
 * \file      main.c
 *
 * \brief     Ping-Pong implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "delay.h"
#include "timer.h"
#include "radio.h"
#include "tremo_system.h"
#include "tremo_gpio.h"

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

#define LORA_BANDWIDTH 0		// [0: 125 kHz,
								//  1: 250 kHz,
								//  2: 500 kHz,
								//  3: Reserved]
#define LORA_SPREADING_FACTOR 7 // [SF7..SF12]
#define LORA_CODINGRATE 1		// [1: 4/5,
								//  2: 4/6,
								//  3: 4/7,
								//  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8	// Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0	// Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false

#elif defined(USE_MODEM_FSK)

#define FSK_FDEV 25000			  // Hz
#define FSK_DATARATE 50000		  // bps
#define FSK_BANDWIDTH 1000000	  // Hz >> DSB in sx126x
#define FSK_AFC_BANDWIDTH 1000000 // Hz
#define FSK_PREAMBLE_LENGTH 5	  // Same for Tx and Rx
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
char sendBuf[] = "hello"; //发送的数据

volatile States_t State = LORA_IDLE;

int8_t RssiValue = 0;
int8_t SnrValue = 0;

uint32_t ChipId[2] = {0};
uint8_t sendMsgFlag = 2; // 0：空闲状态可以发送数据；1：正在发送，等待发送完成；2：发送完成

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
 * Main application entry point.
 */
int Ra08KitLoraTestStart(void)
{
	static uint8_t ledStatus = 0;

	printf("Ra-08-kit test Start!\r\nversion:0.0.0\r\n");

	(void)system_get_chip_id(ChipId);


// mode：工作模式，LoRa芯片可工作在FSK模式/LoRa模式；
// power：LoRa工作功率
// fdev：FSK模式下设置，不考虑使用
// bandwidth：带宽-125K、250K、500K
// datarate：扩频因子
// coderate： 编码率
// preambleLen：前导码长度
// fixLen：数据包长度是否固定
// crcOn：是否启动CRC校验
// freqHopOn：调频使能开关
// hopPeriod：若启动调频，调频周期确定
// iqInverted：反转IQ信号(笔者也没搞过呀)
// timeout：设置超时时间（一般是接收超时时间）

	// Radio initialization
	RadioEvents.TxDone = OnTxDone;
	RadioEvents.RxDone = OnRxDone;
	RadioEvents.TxTimeout = OnTxTimeout;
	RadioEvents.RxTimeout = OnRxTimeout;
	RadioEvents.RxError = OnRxError;

	Radio.Init(&RadioEvents);

	Radio.SetChannel(RF_FREQUENCY);

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

	printf("Lora Mode\r\nversion:0.0.0\r\ntx power:%d\r\nSF:%d\r\nCR:4/%d\r\n", TX_OUTPUT_POWER, LORA_SPREADING_FACTOR, LORA_CODINGRATE + 4);
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
		switch (State)
		{
		case RX:
			printf("[%s()-%d]Rx done,rssi:%d,len:%d,ledStatus:%d,data:%s\r\n", __func__, __LINE__, RssiValue, ledStatus, BufferSize, Buffer);
			switch (ledStatus)
			{
			case 0:
				gpio_write(GPIOA, GPIO_PIN_4, GPIO_LEVEL_HIGH);
				gpio_write(GPIOA, GPIO_PIN_5, GPIO_LEVEL_LOW);
				gpio_write(GPIOA, GPIO_PIN_7, GPIO_LEVEL_LOW);
				ledStatus = 1;
				break;
			case 1:
				gpio_write(GPIOA, GPIO_PIN_4, GPIO_LEVEL_LOW);
				gpio_write(GPIOA, GPIO_PIN_5, GPIO_LEVEL_HIGH);
				gpio_write(GPIOA, GPIO_PIN_7, GPIO_LEVEL_LOW);
				ledStatus = 2;
				break;
			case 2:
			default:
				gpio_write(GPIOA, GPIO_PIN_4, GPIO_LEVEL_LOW);
				gpio_write(GPIOA, GPIO_PIN_5, GPIO_LEVEL_LOW);
				gpio_write(GPIOA, GPIO_PIN_7, GPIO_LEVEL_HIGH);
				ledStatus = 0;
				break;
			}
			Radio.Rx(RX_TIMEOUT_VALUE);
			State = LORA_IDLE;
			break;
		case TX:
			printf("[%s()-%d]Tx done\r\n", __func__, __LINE__);
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
				printf("[%s()-%d]send message:%s\r\n", __func__, __LINE__, sendBuf);
				Radio.Send((uint8_t *)sendBuf, strlen(sendBuf) + 1);
				sendMsgFlag = 1;
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
