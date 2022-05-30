#include<stdio.h>
#include <stdlib.h>
#include "aithinker_code.h"
#include "tremo_rcc.h"
#include "tremo_gpio.h"
#include "timer.h"
#include "tremo_system.h"
#include "rtc-board.h"
#include "lwan_config.h"
#include "linkwan_ica_at.h"

//下面五个宏定义是从 lora\linkwan\linkwan_ica_at.c 文件中摘抄的
#define QUERY_CMD        0x01
#define EXECUTE_CMD        0x02
#define DESC_CMD        0x03
#define SET_CMD            0x04
#define ATCMD_SIZE (255 * 2 + 18)

volatile uint32_t g_aithinkerEvent=0;	//事件状态机，每个位表示一个事件
uint8_t g_ledIndex=0;	//LED亮灯序号,0表示开始1～5表示红绿蓝黄白
static TimerTime_t g_ledStartRtc=0;	//led任务启动时间
static uint8_t g_IoMap[]={255,8,11,9,4,5,7,6,255,255,255,16,15,14,2,255,255,255};	//IO映射表

//--------------------------------- 打印启动信息 ---------------------------------
void AithinkerPrintInfo(void){
	uint32_t sn[2];
	uint8_t *buf = (uint8_t *)sn;

	system_get_chip_id(sn);

	printf("\r\n################################################\r\n");
	printf("\r\n");
	printf("arch:ASR6601,%02X%02X%02X%02X%02X%02X%02X%02X\r\n",buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
	printf("company:Ai-Thinker|B&T\r\n");
	printf("sdk_version:release/v1.5.0\r\n");
	printf("firmware_version:%s\r\n",FARMWARE_VERSION);
	printf("compile_time:%s %s\r\n",__DATE__,__TIME__);
	printf("\r\n");
	printf("ready\r\n");
	printf("\r\n");
	printf("################################################\r\n");
	printf("LoRaWAN for %s\r\n",LORAWAN_REGION_STR);
		
}

//--------------------------- aithinker事件处理函数(这个会轮询调用) ----------------------------
void AithinkerEventProcess(void){
	//处理LED事件
	if(g_aithinkerEvent&EVENT_LEDTEST){
		switch(g_ledIndex){
			case 0:
				g_ledStartRtc=RtcGetTimerValue();
				gpio_set_iomux(GPIOA,GPIO_PIN_7,0);
				g_ledIndex=1;
			case 1:
				gpio_init(GPIOA,GPIO_PIN_5,GPIO_MODE_OUTPUT_PP_HIGH);
				gpio_init(GPIOA,GPIO_PIN_4,GPIO_MODE_OUTPUT_PP_LOW);
				gpio_init(GPIOA,GPIO_PIN_7,GPIO_MODE_OUTPUT_PP_LOW);
				gpio_init(GPIOA,GPIO_PIN_15,GPIO_MODE_OUTPUT_PP_LOW);
				gpio_init(GPIOA,GPIO_PIN_14,GPIO_MODE_OUTPUT_PP_LOW);
				if(RtcGetTimerValue()-g_ledStartRtc>1000){
					g_ledIndex=2;
				}else{
					break;
				}
			case 2:
				gpio_init(GPIOA,GPIO_PIN_5,GPIO_MODE_OUTPUT_PP_LOW);
				gpio_init(GPIOA,GPIO_PIN_4,GPIO_MODE_OUTPUT_PP_HIGH);
				gpio_init(GPIOA,GPIO_PIN_7,GPIO_MODE_OUTPUT_PP_LOW);
				gpio_init(GPIOA,GPIO_PIN_15,GPIO_MODE_OUTPUT_PP_LOW);
				gpio_init(GPIOA,GPIO_PIN_14,GPIO_MODE_OUTPUT_PP_LOW);
				if(RtcGetTimerValue()-g_ledStartRtc>2000){
					g_ledIndex=3;
				}else{
					break;
				}
			case 3:
				gpio_init(GPIOA,GPIO_PIN_5,GPIO_MODE_OUTPUT_PP_LOW);
				gpio_init(GPIOA,GPIO_PIN_4,GPIO_MODE_OUTPUT_PP_LOW);
				gpio_init(GPIOA,GPIO_PIN_7,GPIO_MODE_OUTPUT_PP_HIGH);
				gpio_init(GPIOA,GPIO_PIN_15,GPIO_MODE_OUTPUT_PP_LOW);
				gpio_init(GPIOA,GPIO_PIN_14,GPIO_MODE_OUTPUT_PP_LOW);
				if(RtcGetTimerValue()-g_ledStartRtc>3000){
					g_ledIndex=4;
				}else{
					break;
				}
			case 4:
				gpio_init(GPIOA,GPIO_PIN_5,GPIO_MODE_OUTPUT_PP_LOW);
				gpio_init(GPIOA,GPIO_PIN_4,GPIO_MODE_OUTPUT_PP_LOW);
				gpio_init(GPIOA,GPIO_PIN_7,GPIO_MODE_OUTPUT_PP_LOW);
				gpio_init(GPIOA,GPIO_PIN_15,GPIO_MODE_OUTPUT_PP_HIGH);
				gpio_init(GPIOA,GPIO_PIN_14,GPIO_MODE_OUTPUT_PP_LOW);
				if(RtcGetTimerValue()-g_ledStartRtc>4000){
					g_ledIndex=5;
				}else{
					break;
				}
			case 5:
				gpio_init(GPIOA,GPIO_PIN_5,GPIO_MODE_OUTPUT_PP_LOW);
				gpio_init(GPIOA,GPIO_PIN_4,GPIO_MODE_OUTPUT_PP_LOW);
				gpio_init(GPIOA,GPIO_PIN_7,GPIO_MODE_OUTPUT_PP_LOW);
				gpio_init(GPIOA,GPIO_PIN_15,GPIO_MODE_OUTPUT_PP_LOW);
				gpio_init(GPIOA,GPIO_PIN_14,GPIO_MODE_OUTPUT_PP_HIGH);
				if(RtcGetTimerValue()-g_ledStartRtc>5000){
					g_ledIndex=6;
				}else{
					break;
				}
			default:
				gpio_init(GPIOA,GPIO_PIN_5,GPIO_MODE_OUTPUT_PP_LOW);
				gpio_init(GPIOA,GPIO_PIN_4,GPIO_MODE_OUTPUT_PP_LOW);
				gpio_init(GPIOA,GPIO_PIN_7,GPIO_MODE_OUTPUT_PP_LOW);
				gpio_init(GPIOA,GPIO_PIN_15,GPIO_MODE_OUTPUT_PP_LOW);
				gpio_init(GPIOA,GPIO_PIN_14,GPIO_MODE_OUTPUT_PP_LOW);
				g_aithinkerEvent&=(~EVENT_LEDTEST);
				break;
		}
	}
	//处理其他事件
}

//--------------------------- 自定义AT指令函数 ----------------------------------------------
//设置GPIO输出电平
int at_sysGpioWrite_func(int opt, int argc, char *argv[]){
	int ret = LWAN_ERROR;
	switch(opt) {
		case QUERY_CMD: //查询AT?
			snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\n%s:2\r\nERROR\r\n",LORA_AT_SYSGPIOWRITE);
			AT_PRINTF("%s", g_pAtCmd);
			break;
		case EXECUTE_CMD:	//执行指令AT
			snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\n%s:2\r\nERROR\r\n",LORA_AT_SYSGPIOWRITE);
			AT_PRINTF("%s", g_pAtCmd);
			break;
		case DESC_CMD:	//帮助信息
			ret = LWAN_SUCCESS;
			snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\n%s:Set GPIO out level\r\n  eg:AT%s=<pin>,<level>\r\n",LORA_AT_SYSGPIOWRITE,LORA_AT_SYSGPIOWRITE);
			break;
		case SET_CMD:
			if(argc<2){
				snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\n%s:3\r\nERROR\r\n",LORA_AT_SYSGPIOWRITE);
				AT_PRINTF("%s", g_pAtCmd);
				break;
			}
			int8_t pinSrc = strtol((const char *)argv[0], NULL, 0);
			if(pinSrc>=sizeof(g_IoMap)){//IO越界
				snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\n%s:129\r\nERROR\r\n",LORA_AT_SYSGPIOWRITE);
				AT_PRINTF("%s", g_pAtCmd);
				break;
			}
			if(255==g_IoMap[pinSrc]){//NC引脚
				snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\n%s:130\r\nERROR\r\n",LORA_AT_SYSGPIOWRITE);
				AT_PRINTF("%s", g_pAtCmd);
				break;
			}
			int8_t pinLev = strtol((const char *)argv[1], NULL, 0);
			gpio_mode_t goioMode=GPIO_MODE_OUTPUT_PP_LOW;
			if(pinLev){
				goioMode=GPIO_MODE_OUTPUT_PP_HIGH;
			}
			ret = LWAN_SUCCESS;
			gpio_set_iomux(GPIOA,GPIO_PIN_6,0);
			gpio_set_iomux(GPIOA,GPIO_PIN_7,0);
			if(g_IoMap[pinSrc]<16){
				gpio_init(GPIOA,g_IoMap[pinSrc]%16,goioMode);
			}else{
				gpio_init(GPIOB,g_IoMap[pinSrc]%16,goioMode);
			}
			snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\nOK\r\n");
			break;
		default:
			break;
	}

	return ret;

}

//读取GPIO电平
int at_sysGpioRead_func(int opt, int argc, char *argv[]){
	int ret = LWAN_ERROR;
	switch(opt) {
		case QUERY_CMD: //查询AT?
			snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\n%s:2\r\nERROR\r\n",LORA_AT_SYSGPIOREAD);
			AT_PRINTF("%s", g_pAtCmd);
			break;
		case EXECUTE_CMD:	//执行指令AT
			snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\n%s:2\r\nERROR\r\n",LORA_AT_SYSGPIOREAD);
			AT_PRINTF("%s", g_pAtCmd);
			break;
		case DESC_CMD:	//帮助信息
			ret = LWAN_SUCCESS;
			snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\n%s:Get GPIO level\r\n  eg:AT%s=<pin>\r\n",LORA_AT_SYSGPIOREAD,LORA_AT_SYSGPIOREAD);
			break;
		case SET_CMD:
			if(argc<1){
				snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\n%s:3\r\nERROR\r\n",LORA_AT_SYSGPIOREAD);
				AT_PRINTF("%s", g_pAtCmd);
				break;
			}
			int pinSrc = strtol((const char *)argv[0], NULL, 0);
			if(pinSrc>=sizeof(g_IoMap)){//IO越界
				snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\n%s:129\r\nERROR\r\n",LORA_AT_SYSGPIOREAD);
				AT_PRINTF("%s", g_pAtCmd);
				break;
			}
			if(255==g_IoMap[pinSrc]){//NC引脚
				snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\n%s:130\r\nERROR\r\n",LORA_AT_SYSGPIOREAD);
				AT_PRINTF("%s", g_pAtCmd);
				break;
			}
			ret = LWAN_SUCCESS;
			gpio_set_iomux(GPIOA,GPIO_PIN_6,0);
			gpio_set_iomux(GPIOA,GPIO_PIN_7,0);
			if(g_IoMap[pinSrc]<16){
				gpio_init(GPIOA,g_IoMap[pinSrc]%16,GPIO_MODE_INPUT_PULL_UP);
				snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\n%s:%d,%d\r\nOK\r\n",LORA_AT_SYSGPIOREAD,pinSrc,gpio_read(GPIOA,g_IoMap[pinSrc]%16));
			}else{
				gpio_init(GPIOB,g_IoMap[pinSrc]%16,GPIO_MODE_INPUT_PULL_UP);
				snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\n%s:%d,%d\r\nOK\r\n",LORA_AT_SYSGPIOREAD,pinSrc,gpio_read(GPIOB,g_IoMap[pinSrc]%16));
			}
			break;
		default:
			break;
	}

	return ret;
}

//开启产测任务指令
int at_nodeMcuTest_func(int opt, int argc, char *argv[]){
	int ret = LWAN_ERROR;
	switch(opt) {
		case QUERY_CMD: //查询AT?
			snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\n%s:2\r\nERROR\r\n",LORA_AT_NODEMCUTEST);
			AT_PRINTF("%s", g_pAtCmd);
			break;
		case EXECUTE_CMD:	//执行指令AT
			snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\n%s:2\r\nERROR\r\n",LORA_AT_NODEMCUTEST);
			AT_PRINTF("%s", g_pAtCmd);
			break;
		case DESC_CMD:	//帮助信息
			ret = LWAN_SUCCESS;
			snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\n%s:Set development board test function\r\n  eg:AT%s=0/1\r\n",LORA_AT_NODEMCUTEST,LORA_AT_NODEMCUTEST);
			break;
		case SET_CMD:
			if(argc<1){
				snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\n%s:3\r\nERROR\r\n",LORA_AT_NODEMCUTEST);
				AT_PRINTF("%s", g_pAtCmd);
				break;
			}
			int8_t mode = strtol((const char *)argv[0], NULL, 0);
			ret = LWAN_SUCCESS;
			if(mode){
				rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);
				gpio_init(GPIOA, GPIO_PIN_2, GPIO_MODE_INPUT_PULL_DOWN);
				gpio_config_interrupt(GPIOA, GPIO_PIN_2, GPIO_INTR_RISING_EDGE);

				/* NVIC config */
				NVIC_EnableIRQ(GPIO_IRQn);
				NVIC_SetPriority(GPIO_IRQn, 2);
				g_aithinkerEvent|=EVENT_AITHINKER;
				snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\nOK\r\n");
				break;
			}else{
				snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\nOK\r\n");
				g_aithinkerEvent&=(~EVENT_AITHINKER);
			}
			break;
		default:
			break;
	}

	return ret;
}

//启动跑马灯指令
int at_ledTest_func(int opt, int argc, char *argv[]){
	int ret = LWAN_ERROR;
	switch(opt) {
		case QUERY_CMD: //查询AT?
			snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\n%s:2\r\nERROR\r\n",LORA_AT_LEDTEST);
			AT_PRINTF("%s", g_pAtCmd);
			break;
		case EXECUTE_CMD:	//执行指令AT
			snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\n%s:2\r\nERROR\r\n",LORA_AT_LEDTEST);
			AT_PRINTF("%s", g_pAtCmd);
			break;
		case DESC_CMD:	//帮助信息
			ret = LWAN_SUCCESS;
			snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\n%s:Start test board LED test\r\n  eg:AT%s\r\n",LORA_AT_LEDTEST,LORA_AT_LEDTEST);
			break;
		case SET_CMD:
			if(argc<1){
				snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\n%s:3\r\nERROR\r\n",LORA_AT_LEDTEST);
				AT_PRINTF("%s", g_pAtCmd);
				break;
			}
			ret = LWAN_SUCCESS;
			g_ledIndex=0;
			int8_t mode = strtol((const char *)argv[0], NULL, 0);
			if(mode){
				g_aithinkerEvent|=EVENT_LEDTEST;
			}else{
				gpio_init(GPIOA,GPIO_PIN_7,GPIO_MODE_OUTPUT_PP_LOW);
				gpio_init(GPIOA,GPIO_PIN_4,GPIO_MODE_OUTPUT_PP_LOW);
				gpio_init(GPIOA,GPIO_PIN_5,GPIO_MODE_OUTPUT_PP_LOW);
				gpio_init(GPIOA,GPIO_PIN_14,GPIO_MODE_OUTPUT_PP_LOW);
				gpio_init(GPIOA,GPIO_PIN_15,GPIO_MODE_OUTPUT_PP_LOW);
				g_aithinkerEvent&=(~EVENT_LEDTEST);
			}
			snprintf((char *)g_pAtCmd, ATCMD_SIZE, "\r\nOK\r\n");
			break;
		default:
			break;
	}

	return ret;
}


