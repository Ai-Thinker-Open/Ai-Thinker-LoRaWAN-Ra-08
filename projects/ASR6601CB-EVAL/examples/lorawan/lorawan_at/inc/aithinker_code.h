#ifndef __AITHINKER_CODE_H__
#define __AITHINKER_CODE_H__
#include<stdint.h>

//下面是Aithinker工程添加
//#define FARMWARE_VERSION	"debug/v1.3.4"	//软件版本(这个是替代了 CONFIG_VERSION 的作用)
#define FARMWARE_VERSION	"release/v1.3.4"	//软件版本(这个是替代了 CONFIG_VERSION 的作用)
//LoRaWAN国家设置
#ifdef REGION_EU868
	#define LORAWAN_REGION_STR      "EU868"
#elif defined(REGION_US915)
	#define LORAWAN_REGION_STR	"US915"
#else
	#define LORAWAN_REGION_STR      "CN470"
#endif

//事件状态机，每个位表示一个事件
#define EVENT_AITHINKER	(1<<0)	//开发板测试事件
#define EVENT_LEDTEST	(1<<1)	//LED测试事件

//aithinker 指令
#define LORA_AT_SYSGPIOWRITE "+SYSGPIOWRITE"
#define LORA_AT_SYSGPIOREAD "+SYSGPIOREAD"
#define LORA_AT_NODEMCUTEST "+NodeMCUTEST"
#define LORA_AT_LEDTEST "+LEDTEST"

extern volatile uint32_t g_aithinkerEvent;
extern uint8_t g_ledIndex;
extern uint8_t *g_pAtCmd;

void AithinkerPrintInfo(void);
void AithinkerEventProcess(void);
int at_sysGpioWrite_func(int opt, int argc, char *argv[]);
int at_sysGpioRead_func(int opt, int argc, char *argv[]);
int at_nodeMcuTest_func(int opt, int argc, char *argv[]);
int at_ledTest_func(int opt, int argc, char *argv[]);

#endif	//end of __AITHINKER_CODE_H__
