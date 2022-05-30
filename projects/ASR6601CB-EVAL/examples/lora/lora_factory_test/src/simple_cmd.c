#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<type_define.h>
#include "delay.h"
#include "tremo_rcc.h"
#include "tremo_wdg.h"
#include "tremo_flash.h"
#include "tremo_system.h"
#include "tremo_gpio.h"
#include "sx126x-board.h"
#include "sx126x.h"
#include "radio.h"
#include "simple_cmd.h"

//产测RF通信指令定义
#define LORA_FACTORY_TEST_CMD_QUERY_FREQ	(0x01)	//查询频率指令(待测模组发出)
#define LORA_FACTORY_TEST_CMD_QUERY_FREQ_ACK	(0x81)	//查询频率指令响应(测试底板发出)
#define LORA_FACTORY_TEST_CMD_SET_GPIO	(0x02)	//设置测试底板GPIO电平指令(待测模组发出)
#define LORA_FACTORY_TEST_CMD_SET_GPIO_ACK	(0x82)	//设置测试底板GPIO电平指令响应(测试底板发出)

#define LORA_PREAMBLE_LENGTH       8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT        5         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON       false

//函数/变量声明
static void CmdParse(void);
static int AI_ParseParam(char *buf,char **argv,uint8_t maxArgc);
static uint8_t LoRaTestInit(void);
static void FactoryTestOnTxDone(void);
static void FactoryTestOnRxDone(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr);
static void FactoryTestOnTxTimeout(void);
static void FactoryTestOnRxTimeout(void);
static void FactoryTestOnRxError(void);
//AT指令函数声明
static int SimpleCmd_At(int argc, char* argv[]);
static int SimpleCmd_AtCgsn(int argc, char* argv[]);
static int SimpleCmd_AtMode(int argc, char* argv[]);
static int SimpleCmd_AtQueryFreq(int argc, char* argv[]);
static int SimpleCmd_AtSetGpioLev(int argc, char* argv[]);
static int SimpleCmd_AtSetRf(int argc, char* argv[]);
//事件处理函数声明
static uint8_t GpioSetInput(uint8_t lev);
static void GpioGetInput(uint8_t *pBuf);
static void ProcessQueryFreqEvent(void);
static void ProcessSetGpioEvent(void);

//数据类型定义
typedef int(*SimpleATHandler)(int argc, char* argv[]);
typedef struct{
    char name[32];
    SimpleATHandler fn;
}SimpleCmdCaseSt;

//指令列表定义
SimpleCmdCaseSt g_cmdList[]={
	//公用指令
	{"AT",SimpleCmd_At},
	{"AT+CGSN?",SimpleCmd_AtCgsn},
	{"AT+MODE",SimpleCmd_AtMode},
	//待测模组指令
	{"AT+QUERYFREQ",SimpleCmd_AtQueryFreq},
	{"AT+SETGPIOLEV",SimpleCmd_AtSetGpioLev},
	//测试底板指令
	{"AT+SETRF",SimpleCmd_AtSetRf},
};

//变量定义
SimpleCmd_s g_cmdStatus;	//记录运行状态的结构体
static int g_cmdSize=0;	//g_cmdList 长度
SimpleCmdConfig g_simpleCmdConfig;	//flash保存的配置变量

static RadioEvents_t g_FactoryTestRadioEvents;

//----------------------------------------- 外部调用函数 -----------------------
//启动指令任务(该任务不会返回)
void SimpleCmdStart(void){
	uint32_t sn[2];

	//初始化框架变量
	memset(&g_cmdStatus,0,sizeof(SimpleCmd_s));
	g_cmdSize=sizeof(g_cmdList)/sizeof(g_cmdList[0]);
	//初始化chip ID	
	system_get_chip_id(sn);
	memcpy(g_cmdStatus.sn,&sn,8);
	GpioSetInput(0);//将除了log串口之外的所有GPIO都设置为下拉输入模式
	LoRaTestInit();	//LoRa初始化
	
	while(1){//这个是main函数的while大循环
		//printf("[%s()-%d]g_eventFlag=0x%lx\r\n",__func__,__LINE__,g_cmdStatus.eventFlag);
		wdg_reload();	//喂狗
		Radio.IrqProcess();	//处理LoRa事件

		//处理AT指令
		if(g_cmdStatus.eventFlag & EVENT_CMD_START){
			CmdParse();
			memset((uint8_t *)g_cmdStatus.reciveBuf,0,SIMPLE_CMD_RX_BUF_SIZE);
			g_cmdStatus.reciveCount=0;
			g_cmdStatus.eventFlag^=EVENT_CMD_START;
		}

		ProcessQueryFreqEvent();	//查询SF任务事件
		ProcessSetGpioEvent();	//处理GPIO设置事件
    }
}

//保存flash
uint8_t SimpleCmdSaveConfig(void){
	flash_erase_page(SIMPLE_CMD_CONFIG_FLASH_ADDR);
    flash_program_bytes(SIMPLE_CMD_CONFIG_FLASH_ADDR,(uint8_t*)(&g_simpleCmdConfig),sizeof(SimpleCmdConfig));
	return 0;
}

//加载配置文件
//返回值
//    0：加载成功
//    1：flash为空，使用了默认值，并保存到flash
uint8_t SimpleCmdLoadConfig(void){
	memcpy(&g_simpleCmdConfig,(uint8_t*)(SIMPLE_CMD_CONFIG_FLASH_ADDR),sizeof(SimpleCmdConfig));
	if(0x55==g_simpleCmdConfig.saveFlag){
		//printf("[%s()-%d]load flash success\r\n",__func__,__LINE__);
		return 0;
	}else{
		printf("[%s()-%d]load flash error,use default config\r\n",__func__,__LINE__);
		memset(&g_simpleCmdConfig,0,sizeof(SimpleCmdConfig));
		g_simpleCmdConfig.testBoardFreq=430000000;
		g_simpleCmdConfig.testBoardPower=2;
		g_simpleCmdConfig.ackRssiOffset=0;
		g_simpleCmdConfig.testBoardBw=1;
		g_simpleCmdConfig.testBoardSf=7;
		g_simpleCmdConfig.testBoardCr=1;
		for(int i=0;i<TEST_MACHINE_MAX_NUMBER;i++){
			g_simpleCmdConfig.testBoardStartFreq[i]=430000000+i*10123000;
		}
		g_simpleCmdConfig.saveFlag=0x55;
		SimpleCmdSaveConfig();
		printf("[%s()-%d]write flash end\r\n",__func__,__LINE__);
		return 1;
	}
}

//获取当前经过的时间单位ms(这个是基于systick的，所以在中断中是无效的)
//参数：bak：时间的起记录的systick
//返回值：经过的时间(ms)
//eg:g_sysTick=1时记录下bak=g_sysTick=1；当g_sysTick=10的时候返回值是9，表示距离bak已经经过了9个systick
uint32_t GetTimePassedMs(uint32_t bak){
	uint32_t now=g_sysTick;

	if(now<bak){
		//溢出了
		return (0xffffffff-bak+now);
	}else{
		//没有溢出
		return (now-bak);
	}
}

//---------------------------------------- AT指令 ---------------------------------------
static int SimpleCmd_At(int argc, char* argv[]){
	printf("[%s()-%d]AT test,param:\r\n",__func__,__LINE__);
	for(int i=0;i<argc;i++){
		printf("argv[%d]=-%s-\r\n",i,argv[i]);
	}

	return 0;
}

//查询chip id
static int SimpleCmd_AtCgsn(int argc, char* argv[]){
	printf("\r\n+CGSN=%02X%02X%02X%02X%02X%02X%02X%02X\r\nOK\r\n",g_cmdStatus.sn[0],g_cmdStatus.sn[1],g_cmdStatus.sn[2],g_cmdStatus.sn[3],g_cmdStatus.sn[4],g_cmdStatus.sn[5],g_cmdStatus.sn[6],g_cmdStatus.sn[7]);
	return 0;
}

//设置模组模式
//AT+MODE 查询
//AT+MODE=0	设置为待测模组
//AT+MODE=1 设置为测试底板
static int SimpleCmd_AtMode(int argc, char* argv[]){
	if(1==argc){
		if(g_simpleCmdConfig.isTestBoard){
			//是测试底板
			printf("\r\ntest board\r\nOK\r\n");
		}else{
			//是待测模组
			printf("\r\nmodule\r\nOK\r\n");
		}
		return 0;
	}
	if(2==argc){
		if('0'==argv[1][0]){
			//设置为待测模组
			g_simpleCmdConfig.isTestBoard=0;
		}else{
			//设置为测试底板
			g_simpleCmdConfig.isTestBoard=1;
		}
		SimpleCmdSaveConfig();
		printf("\r\nsave mode=%d,reset mcu\r\nOK\r\n",g_simpleCmdConfig.isTestBoard);
		DelayMs(10);
		system_reset();	//复位
		while(1);
	}
	printf("\r\ncmd error\r\nERROR\r\n");
	return -1;
}

//开启/关闭查询频率任务
//根据参数开启查询SF任务
//AT+QUERYFREQ=freqOffset,power,bw,sf,cr
static int SimpleCmd_AtQueryFreq(int argc, char* argv[]){
	if(g_simpleCmdConfig.isTestBoard){
		//是测试底板
		printf("\r\nneed in module mode\r\nERROR\r\n");
		return -1;
	}
	//判断参数个数
	if(argc!=6){
		printf("\r\nuseage:AT+QUERY=freqOffset,fre,power,bw,cr\r\n    freqOffset:\r\n    power:2~22\r\n    bw:0:125k\r\n       1:250k\r\n       2:500k\r\n    sf:7~12\r\n    cr:1:4/5\r\n       2:4/6\r\n       3:4/7\r\n       4:4/8\r\nERROR\r\n");
		return -2;
	}

	//启动频率查询任务
	g_cmdStatus.moduleFreqOffset=atoi(argv[1]);
	if(g_cmdStatus.moduleFreqOffset<0){
		printf("\r\nfreqOffset must >0\r\nERROR\r\n");
		return -3;
	}
	g_cmdStatus.modulePower=atoi(argv[2]);
	if(g_cmdStatus.modulePower<2 || g_cmdStatus.modulePower>22){
		printf("\r\npower must in 2~22\r\nERROR\r\n");
		return -4;
	}
	g_cmdStatus.moduleBw=atoi(argv[3]);
	if(g_cmdStatus.moduleBw<0 || g_cmdStatus.moduleBw>2){
		printf("\r\nbw must in 0~2\r\nERROR\r\n");
		return -5;
	}
	g_cmdStatus.moduleSf=atoi(argv[4]);
	if(g_cmdStatus.moduleSf<7 || g_cmdStatus.moduleSf>12){
		printf("\r\nsf must in 7~12\r\nERROR\r\n");
		return -6;
	}
	g_cmdStatus.moduleCr=atoi(argv[5]);
	if(g_cmdStatus.moduleCr<1 || g_cmdStatus.moduleCr>4){
		printf("\r\ncr must in 1~4\r\nERROR\r\n");
		return -7;
	}
	g_cmdStatus.eventQueryFreq=EVENT_QUERY_FREQ_INIT;
	printf("\r\nOK\r\n");
	return 0;
}

//设置测试底板的GPIO(待测模组发起)
//AT+SETGPIOLEV=Freq,power,bw,sf,cr,pin,lev
//    fre:150000000-960000000
//    power:2~22
//    bw:0:125k
//       1:250k
//       2:500k
//    sf:7~12
//    cr:1:4/5
//       2:4/6
//       3:4/7
//       4:4/8
//    pin:GPIO序号0~63
//    lev:设置GPIO电平0：低电平，1高电平
static int SimpleCmd_AtSetGpioLev(int argc, char* argv[]){
	int pin=0,lev=0;
	
	if(g_simpleCmdConfig.isTestBoard){
		//是测试底板
		printf("\r\nneed in module mode\r\nERROR\r\n");
		return -1;
	}
	//判断参数个数
	if(argc!=8){
		printf("\r\nuseage:AT+SETGPIOLEV=Freq,power,bw,sf,cr,pin,lev\r\n    fre:150000000-960000000\r\n    power:2~22\r\n    bw:0:125k\r\n       1:250k\r\n       2:500k\r\n    sf:7~12\r\n    cr:1:4/5\r\n       2:4/6\r\n       3:4/7\r\n       4:4/8\r\n    pin:0~63\r\n    lev:0/1\r\nERROR\r\n");
		return -2;
	}
	//参数个数正确，开始解析参数
	g_cmdStatus.moduleFreq=atoi(argv[1]);
	if(g_cmdStatus.moduleFreq<150000000 || g_cmdStatus.moduleFreq>960000000){
		printf("\r\nfre must in 150000000~960000000\r\nERROR\r\n");
		return -3;
	}
	g_cmdStatus.modulePower=atoi(argv[2]);
	if(g_cmdStatus.modulePower<2 || g_cmdStatus.modulePower>22){
		printf("\r\npower must in 2~22\r\nERROR\r\n");
		return -4;
	}
	g_cmdStatus.moduleBw=atoi(argv[3]);
	if(g_cmdStatus.moduleBw<0 || g_cmdStatus.moduleBw>2){
		printf("\r\nbw must in 0~2\r\nERROR\r\n");
		return -5;
	}
	g_cmdStatus.moduleSf=atoi(argv[4]);
	if(g_cmdStatus.moduleSf<7 || g_cmdStatus.moduleSf>12){
		printf("\r\nsf must in 7~12\r\nERROR\r\n");
		return -6;
	}
	g_cmdStatus.moduleCr=atoi(argv[5]);
	if(g_cmdStatus.moduleCr<1 || g_cmdStatus.moduleCr>4){
		printf("\r\ncr must in 1~4\r\nERROR\r\n");
		return -7;
	}
	pin=atoi(argv[6]);
	if(pin<0 || pin>63){
		printf("\r\npin must in 0~63\r\nERROR\r\n");
		return -8;
	}
	lev=atoi(argv[7]);
	//设置RF参数
	Radio.SetChannel(g_cmdStatus.moduleFreq);
	Radio.SetRxConfig(MODEM_LORA,g_cmdStatus.moduleBw,g_cmdStatus.moduleSf,g_cmdStatus.moduleCr,0,LORA_PREAMBLE_LENGTH,
					LORA_SYMBOL_TIMEOUT,LORA_FIX_LENGTH_PAYLOAD_ON,0,true,0,0,LORA_IQ_INVERSION_ON,true);
	Radio.SetTxConfig(MODEM_LORA,g_cmdStatus.modulePower,0,g_cmdStatus.moduleBw,g_cmdStatus.moduleSf,g_cmdStatus.moduleCr,
					LORA_PREAMBLE_LENGTH,LORA_FIX_LENGTH_PAYLOAD_ON,true,0,0,LORA_IQ_INVERSION_ON,60000);
	//填充发送数据
	g_cmdStatus.txBuf[0]=LORA_FACTORY_TEST_CMD_SET_GPIO;
	memcpy(g_cmdStatus.txBuf+1,g_cmdStatus.sn,8);
	g_cmdStatus.txBuf[9]=6;
	memcpy(g_cmdStatus.txBuf+10,&g_cmdStatus.moduleFreq,4);
	g_cmdStatus.txBuf[14]=pin;
	g_cmdStatus.txBuf[15]=lev;
	
	Radio.Standby();
	g_cmdStatus.eventSetGpio=EVENT_SET_GPIO_WAIT_ACK;
	Radio.Send(g_cmdStatus.txBuf,16);	//发送查询包
	printf("\r\nOK\r\n");
	return 0;
}

//查询和设置RF参数
//查询RF配置
//AT+SETRF
//设置
//AT+SETRF=fre,power,bw,sf,cr
//    fre:150000000-960000000
//    power:2~22
//    bw:0:125k
//       1:250k
//       2:500k
//    sf:7~12
//    cr:1:4/5
//       2:4/6
//       3:4/7
//       4:4/8
static int SimpleCmd_AtSetRf(int argc, char* argv[]){
	int fre,power,offSet,bw,sf,cr;
	if(!g_simpleCmdConfig.isTestBoard){
		//是待测模组
		printf("\r\nneed in test board mode\r\nERROR\r\n");
		return -1;
	}
	if(1==argc){
		//查询指令
		printf("\r\nRF config:\r\n    freq:%d\r\n    power:%d\r\n    power:%d\r\n",g_simpleCmdConfig.testBoardFreq,g_simpleCmdConfig.testBoardPower,g_simpleCmdConfig.ackRssiOffset);
		switch(g_simpleCmdConfig.testBoardBw){
			case 0:
				printf("    bw:125k\r\n");
				break;
			case 1:
				printf("    bw:250k\r\n");
				break;
			case 2:
				printf("    bw:500k\r\n");
				break;
			default:
				printf("    bw:error(%d)\r\n",g_simpleCmdConfig.testBoardBw);
				break;
		}
		printf("    sf:%d\r\n",g_simpleCmdConfig.testBoardSf);
		switch(g_simpleCmdConfig.testBoardCr){
			case 1:
				printf("    cr:[4/5]\r\n");
				break;
			case 2:
				printf("    cr:[4/6]\r\n");
				break;
			case 3:
				printf("    cr:[4/7]\r\n");
				break;
			case 4:
				printf("    cr:[4/8]\r\n");
				break;
			default:
				printf("    cr:error(%d)\r\n",g_simpleCmdConfig.testBoardCr);
				break;
		}
		printf("OK\r\n");
		return 0;
	}
	//判断参数个数
	if(argc!=7){
		printf("\r\nuseage:AT+SETRF=fre,power,ackRssiOffset,bw,sf,cr\r\n    fre:150000000-960000000\r\n    power:2~22\r\n    ackRssiOffset:-20~20\r\n    bw:0:125k\r\n       1:250k\r\n       2:500k\r\n    sf:7~12\r\n    cr:1:4/5\r\n       2:4/6\r\n       3:4/7\r\n       4:4/8\r\nERROR\r\n");
		return -2;
	}
	//参数个数正确，开始解析参数
	fre=atoi(argv[1]);
	if(fre<150000000 || fre>960000000){
		printf("\r\nfre must in 150000000~960000000\r\nERROR\r\n");
		return -3;
	}
	power=atoi(argv[2]);
	if(power<2 || power>22){
		printf("\r\npower must in 2~22\r\nERROR\r\n");
		return -4;
	}
	offSet=atoi(argv[3]);
	if(offSet<-20 || offSet>20){
		printf("\r\noffSet must in -20~20\r\nERROR\r\n");
		return -8;
	}
	bw=atoi(argv[4]);
	if(bw<0 || bw>2){
		printf("\r\nbw must in 0~2\r\nERROR\r\n");
		return -5;
	}
	sf=atoi(argv[5]);
	if(sf<7 || sf>12){
		printf("\r\nsf must in 7~12\r\nERROR\r\n");
		return -6;
	}
	cr=atoi(argv[6]);
	if(cr<1 || cr>4){
		printf("\r\ncr must in 1~4\r\nERROR\r\n");
		return -7;
	}
	g_simpleCmdConfig.testBoardFreq=fre;
	g_simpleCmdConfig.testBoardPower=power;
	g_simpleCmdConfig.ackRssiOffset=offSet;
	g_simpleCmdConfig.testBoardBw=bw;
	g_simpleCmdConfig.testBoardSf=sf;
	g_simpleCmdConfig.testBoardCr=cr;
	SimpleCmdSaveConfig();	//保存到flash

	printf("\r\nset RF config:\r\n    freq:%d\r\n    power:%d\r\n    ackRssiOffset:%d\r\n",g_simpleCmdConfig.testBoardFreq,g_simpleCmdConfig.testBoardPower,g_simpleCmdConfig.ackRssiOffset);
	switch(g_simpleCmdConfig.testBoardBw){
		case 0:
			printf("    bw:125k\r\n");
			break;
		case 1:
			printf("    bw:250k\r\n");
			break;
		case 2:
			printf("    bw:500k\r\n");
			break;
		default:
			printf("    bw:error(%d)\r\n",g_simpleCmdConfig.testBoardBw);
			break;
	}
	printf("    sf:%d\r\n",g_simpleCmdConfig.testBoardSf);
	switch(g_simpleCmdConfig.testBoardCr){
		case 1:
			printf("    cr:[4/5]\r\n");
			break;
		case 2:
			printf("    cr:[4/6]\r\n");
			break;
		case 3:
			printf("    cr:[4/7]\r\n");
			break;
		case 4:
			printf("    cr:[4/8]\r\n");
			break;
		default:
			printf("    cr:error(%d)\r\n",g_simpleCmdConfig.testBoardCr);
			break;
	}
	printf("set success,wait restart\r\nOK\r\n");
	DelayMs(10);
	system_reset(); //复位
	while(1);
}

//---------------------------------------- lora相关 ---------------------------------------
//lora初始化
static uint8_t LoRaTestInit(void){
	g_FactoryTestRadioEvents.TxDone    = FactoryTestOnTxDone;
    g_FactoryTestRadioEvents.RxDone    = FactoryTestOnRxDone;
    g_FactoryTestRadioEvents.TxTimeout = FactoryTestOnTxTimeout;
    g_FactoryTestRadioEvents.RxTimeout = FactoryTestOnRxTimeout;
    g_FactoryTestRadioEvents.RxError   = FactoryTestOnRxError;
	Radio.Init(&g_FactoryTestRadioEvents);
    Radio.SetChannel(g_simpleCmdConfig.testBoardFreq);
    Radio.SetRxConfig(MODEM_LORA,g_simpleCmdConfig.testBoardBw,g_simpleCmdConfig.testBoardSf,g_simpleCmdConfig.testBoardCr,0,LORA_PREAMBLE_LENGTH,
                      LORA_SYMBOL_TIMEOUT,LORA_FIX_LENGTH_PAYLOAD_ON,0,true,0,0,LORA_IQ_INVERSION_ON,true);
	Radio.SetTxConfig(MODEM_LORA,g_simpleCmdConfig.testBoardPower,0,g_simpleCmdConfig.testBoardBw,g_simpleCmdConfig.testBoardSf,g_simpleCmdConfig.testBoardCr,
                      LORA_PREAMBLE_LENGTH,LORA_FIX_LENGTH_PAYLOAD_ON,true,0,0,LORA_IQ_INVERSION_ON,60000);
	if(g_simpleCmdConfig.isTestBoard){
		//测试底板,默认进入rx模式
		Radio.Rx(0);
		printf("[%s()-%d]LoRa init ok,enter rx mode\r\n",__func__,__LINE__);
	}
	return 0;
}

static void FactoryTestOnTxDone(void){
    printf("[%s()-%d]\r\n",__func__,__LINE__);
	if(g_simpleCmdConfig.isTestBoard){
		//测试底板执行的代码
		Radio.Rx(0);
	}else{
		//待测模组执行的代码
		if(EVENT_QUERY_FREQ_WAIT_ACK==g_cmdStatus.eventQueryFreq){
			//查询频率指令发送完成，进入接受模式等待数据返回
			Radio.Rx(0);
		}
		if(EVENT_SET_GPIO_WAIT_ACK==g_cmdStatus.eventSetGpio){
			//设置GPIO指令发送完成，进入接受模式等待数据返回
			Radio.Rx(0);
		}
	}
}

static void FactoryTestOnRxDone(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr){
	INT16 sendRssi=0;	//接收到的数据中携带的RSSI(这个记录的是我们发送出去的RSSI)
	int sendFreq=0;	//接收到的数据中携带的频率(这个记录的是发送方实际设置的频率)
	gpio_mode_t pullMode=GPIO_MODE_OUTPUT_PP_LOW;
	gpio_t* gpiox=NULL;
	uint8_t gpioLev[8]={0};

    printf("[%s()-%d]recive rssi=%d size=%d\r\n",__func__,__LINE__,rssi,size);
	printf("    playload:");
	for(int i=0;i<size;i++){
		printf("%02X ",payload[i]);
	}
	printf("\r\n");

	if(g_simpleCmdConfig.isTestBoard){
		//测试底板执行的代码
		switch(payload[0]){
			case LORA_FACTORY_TEST_CMD_QUERY_FREQ:	//测试底板收到了模组的查询频率指令
				if(14!=size){
					printf("[%s()-%d]recive size=%d is error\r\n",__func__,__LINE__,size);
					return;
				}
				if(0x04!=payload[9]){
					//负载长度错误,返回等待下一个数据
					printf("[%s()-%d]payload:%d is error\r\n",__func__,__LINE__,payload[9]);
					return;
				}
				memcpy(&sendFreq,payload+10,4);
				if(sendFreq!=g_simpleCmdConfig.testBoardFreq){
					//频率错误,返回等待下一个数据
					printf("[%s()-%d]reciveFreq:%d is error\r\n",__func__,__LINE__,sendFreq);
					return;
				}
				sendRssi=rssi;
				//数据正确，构造频率查询ack数据包
				g_cmdStatus.txBuf[0]=LORA_FACTORY_TEST_CMD_QUERY_FREQ_ACK;
				memcpy(g_cmdStatus.txBuf+1,payload+1,8);
				g_cmdStatus.txBuf[9]=6;
				memcpy(g_cmdStatus.txBuf+10,&sendRssi,2);
				memcpy(g_cmdStatus.txBuf+12,&g_simpleCmdConfig.testBoardFreq,4);
				Radio.Standby();
				printf("\r\n[QueryFreqEvent]send ack query freq=%d\r\n",g_simpleCmdConfig.testBoardFreq);
				Radio.Send(g_cmdStatus.txBuf,16);	//发送查询包
				return;
			case LORA_FACTORY_TEST_CMD_SET_GPIO:	//测试底板收到了设置GPIO指令
				if(16!=size){
					printf("[%s()-%d]recive size=%d is error\r\n",__func__,__LINE__,size);
					return;
				}
				if(0x06!=payload[9]){
					//负载长度错误,返回等待下一个数据
					printf("[%s()-%d]payload:%d is error\r\n",__func__,__LINE__,payload[9]);
					return;
				}
				memcpy(&sendFreq,payload+10,4);
				if(sendFreq!=g_simpleCmdConfig.testBoardFreq){
					//频率错误,返回等待下一个数据
					printf("[%s()-%d]reciveFreq:%d is error\r\n",__func__,__LINE__,sendFreq);
					return;
				}
				//设置GPIO
				//payload[14]	pin
				//payload[15]	lev
				GpioSetInput(0);
				switch(payload[14]/16){
					case 0:
						gpiox=GPIOA;
						break;
					case 1:
						gpiox=GPIOB;
						break;
					case 2:
						gpiox=GPIOC;
						break;
					case 3:
						gpiox=GPIOD;
						break;
					default:
						gpiox=GPIOA;
						printf("[%s()-%d]gpio pin:%d is error\r\n",__func__,__LINE__,payload[14]);
						return;
				}
				if(payload[15]){
					pullMode=GPIO_MODE_OUTPUT_PP_HIGH;
				}
				gpio_init(gpiox,payload[14]%16,pullMode);	//设置GPIO电平

				//数据正确，构造GPIO设置ack数据包
				g_cmdStatus.txBuf[0]=LORA_FACTORY_TEST_CMD_SET_GPIO_ACK;
				memcpy(g_cmdStatus.txBuf+1,payload+1,8);
				g_cmdStatus.txBuf[9]=7;
				sendRssi=rssi+g_simpleCmdConfig.ackRssiOffset;
				memcpy(g_cmdStatus.txBuf+10,&sendRssi,2);
				memcpy(g_cmdStatus.txBuf+12,&g_simpleCmdConfig.testBoardFreq,4);
				g_cmdStatus.txBuf[16]=0;
				Radio.Standby();
				printf("\r\n[SetGPIOEvent]send ack;ackRssiOffset=%d sendRssi=%d\r\n",g_simpleCmdConfig.ackRssiOffset,sendRssi);
				Radio.Send(g_cmdStatus.txBuf,17);	//发送响应包
				return;
			default:
				printf("[%s()-%d]unknown cmd:0x%02x\r\n",__func__,__LINE__,payload[0]);
				return;
		}
		return;
	}else{
		//待测模组执行的代码
		if(EVENT_QUERY_FREQ_WAIT_ACK==g_cmdStatus.eventQueryFreq){
			if(16!=size){
				printf("[%s()-%d]recive size=%d is error\r\n",__func__,__LINE__,size);
				return;
			}
			//查询SF指令发送完成，收到了测试底板返回的响应数据，处理SF查询响应数据
			if(LORA_FACTORY_TEST_CMD_QUERY_FREQ_ACK!=payload[0]){
				//响应指令错误，返回等待下一个数据
				printf("[%s()-%d]recive cmd=0x%02x is error\r\n",__func__,__LINE__,payload[0]);
				return;
			}
			if(memcmp(payload+1,g_cmdStatus.sn,8)){
				//sn不同，这个数据不是我们发出的,返回等待下一个数据
				printf("[%s()-%d]recive sn:%02x%02x%02x%02x%02x%02x%02x%02x is error\r\n",__func__,__LINE__,payload[1],payload[2],payload[3],payload[4],payload[5],payload[6],payload[7],payload[8]);
				return;
			}
			if(0x06!=payload[9]){
				//负载长度错误,返回等待下一个数据
				printf("[%s()-%d]payload:%d is error\r\n",__func__,__LINE__,payload[9]);
				return;
			}
			memcpy(&sendRssi,payload+10,2);
			memcpy(&sendFreq,payload+12,4);
			if(sendFreq!=g_cmdStatus.moduleFreq){
				//频率错误,返回等待下一个数据
				printf("[%s()-%d]reciveFreq:%d is error\r\n",__func__,__LINE__,sendFreq);
				return;
			}
			//数据正确，打印接收到的数据
			printf("\r\n[QueryFreqEvent]recive ack ReciveRssi:%d SendRssi:%d SendFreq=%d\r\n",rssi,sendRssi,sendFreq);
			//开始轮询下一个SF
			g_cmdStatus.eventQueryFreq=EVENT_QUERY_FREQ_NEXT_FREQ;
		}
		if(EVENT_SET_GPIO_WAIT_ACK==g_cmdStatus.eventSetGpio){
			if(17!=size){
				printf("[%s()-%d]recive size=%d is error\r\n",__func__,__LINE__,size);
				return;
			}
			//查询SF指令发送完成，收到了测试底板返回的响应数据，处理SF查询响应数据
			if(LORA_FACTORY_TEST_CMD_SET_GPIO_ACK!=payload[0]){
				//响应指令错误，返回等待下一个数据
				printf("[%s()-%d]recive cmd=0x%02x is error\r\n",__func__,__LINE__,payload[0]);
				return;
			}
			if(memcmp(payload+1,g_cmdStatus.sn,8)){
				//sn不同，这个数据不是我们发出的,返回等待下一个数据
				printf("[%s()-%d]recive sn:%02x%02x%02x%02x%02x%02x%02x%02x is error\r\n",__func__,__LINE__,payload[1],payload[2],payload[3],payload[4],payload[5],payload[6],payload[7],payload[8]);
				return;
			}
			if(7!=payload[9]){
				//负载长度错误,返回等待下一个数据
				printf("[%s()-%d]payload:%d is error\r\n",__func__,__LINE__,payload[9]);
				return;
			}
			memcpy(&sendRssi,payload+10,2);
			memcpy(&sendFreq,payload+12,4);
			if(sendFreq!=g_cmdStatus.moduleFreq){
				//频率错误,返回等待下一个数据
				printf("[%s()-%d]reciveFreq:%d is error\r\n",__func__,__LINE__,sendFreq);
				return;
			}
			GpioSetInput(0);	//设置输入
			GpioGetInput(gpioLev);	//读取GPIO电平
			//数据正确，打印接收到的数据
			printf("\r\n[SetGPIOEvent]recive ack ReciveRssi:%d SendRssi:%d SendFreq=%d GPIO_LEV=%02x%02x%02x%02x%02x%02x%02x%02x\r\n",rssi,sendRssi,sendFreq,gpioLev[0],gpioLev[1],gpioLev[2],gpioLev[3],gpioLev[4],gpioLev[5],gpioLev[6],gpioLev[7]);
			//接收成功，停止事件
			Radio.Standby();
			g_cmdStatus.eventSetGpio=EVENT_SET_GPIO_NOT_RUN;
		}
	}
}

static void FactoryTestOnTxTimeout(void){
    printf("[%s()-%d]\r\n",__func__,__LINE__);
}

static void FactoryTestOnRxTimeout(void){
    printf("[%s()-%d]rx timeout restart rx\r\n",__func__,__LINE__);
	Radio.Rx(0);
}

static void FactoryTestOnRxError(void){
    printf("[%s()-%d]rx error restart rx\r\n",__func__,__LINE__);
	Radio.Rx(0);
}

//---------------------------------------- 内部调用函数 ---------------------------------------
static void CmdParse(void){
	char *argv[16]={NULL},*p_str=NULL;
	int argc=0;
	
	//printf("[%s()-%d]g_cmdSize=%d\r\n",__func__,__LINE__,g_cmdSize);
	//去除头部的空格和''\r','\n'
	for(int i=0;i<g_cmdStatus.reciveCount;i++){
		if(' '!=g_cmdStatus.reciveBuf[i] && '\r'!=g_cmdStatus.reciveBuf[i] && '\n'!=g_cmdStatus.reciveBuf[i]){
			argv[0]=(char *)(g_cmdStatus.reciveBuf+i);
			break;
		}
	}
	if(NULL==argv[0]){
		printf("[%s()-%d]cmd error %s\r\n",__func__,__LINE__,g_cmdStatus.reciveBuf);
		return;
	}
	//将尾部的空格和\r\n换成\0
	p_str=(char *)(g_cmdStatus.reciveBuf+g_cmdStatus.reciveCount-1);
	while(p_str-argv[0]>=0){
		if(' '==(*p_str) || '\r'==(*p_str) || '\n'==(*p_str)){
			(*p_str)='\0';
			p_str--;
		}else{
			break;
		}
	}
	//获取指令名称结束位置
	for(int i=0;i<=strlen(argv[0]);i++){
		if('='==argv[0][i] || '\0'==argv[0][i]){
			argv[0][i]='\0';
			argv[1]=argv[0]+i+1;
			break;
		}
	}
	if(NULL==argv[1]){
		printf("[%s()-%d]cmd error %s\r\n",__func__,__LINE__,g_cmdStatus.reciveBuf);
		return;
	}
	//获取参数列表
	//查找命令
	for(int i=0;i<g_cmdSize;i++){
		if(0!=strcasecmp(argv[0],g_cmdList[i].name)){
			continue;
		}
		//找到对应指令了，获取参数列表
		if(0==strlen(argv[1])){
			argc=1;
			argv[1]=NULL;
		}else{
			argc=AI_ParseParam(argv[1],argv+1,sizeof(argv)/sizeof(argv[0])-1);
			argc++;
		}
		g_cmdList[i].fn(argc,argv);
		//printf("[%s()-%d]g_cmdSize=%d i=%d\r\n",__func__,__LINE__,g_cmdSize,i);
		return;
	}
	printf("[%s()-%d]unknown cmd:%s\r\n",__func__,__LINE__,g_cmdStatus.reciveBuf);
}

//将字符串解析为参数(参数使用逗号分隔,第一个和最后一个参数不能为空)
//参数
//    buf：命令字符串
//    argv：参数数组(需要外部提前申请好空间)
//    maxArgc：argv长度
//
//char *argv[MAX_ARGC] = {NULL};
//int argc = AI_ParseParam(argStr, argv,MAX_ARGC);
static int AI_ParseParam(char *buf,char **argv,uint8_t maxArgc){
	int argc = 0;
	char str_buf[SIMPLE_CMD_RX_BUF_SIZE];
	int str_count = 0;
	int buf_cnt = 0;

	memset(str_buf, 0, SIMPLE_CMD_RX_BUF_SIZE);
	if(buf == NULL){
		goto exit;
	}
	
	while((argc < maxArgc) && (*buf != '\0')) {
		while((*buf == ',') || (*buf == '[') || (*buf == ']')){
			if((*buf == ',') && (*(buf+1) == ',')){
				argv[argc] = NULL;
				argc++;
			}
			*buf = '\0';
			buf++;
		}

		if(*buf == '\0'){
			break;
		}else if(*buf == '"'){
			memset(str_buf,'\0',SIMPLE_CMD_RX_BUF_SIZE);
			str_count = 0;
			buf_cnt = 0;
			*buf = '\0';
			buf ++;         
			if(*buf == '\0'){
				break;
			}
			argv[argc] = buf;
			while((*buf != '"')&&(*buf != '\0')){
				if(*buf == '\\'){
					buf ++;
					buf_cnt++;
				}
				str_buf[str_count] = *buf;
				str_count++;
				buf_cnt++;
				buf ++;
			}
			*buf = '\0';
			memcpy(buf-buf_cnt,str_buf,buf_cnt);
		}else{
			argv[argc] = buf;
		}
		argc++;
		buf++;

		while( (*buf != ',')&&(*buf != '\0')&&(*buf != '[')&&(*buf != ']') ){
			buf++;
		}
	}
exit:
	return argc;
}

//将除了log串口(IO16/17)之外的所有GPIO都设置为输入模式
//参数：lev：0：下拉，1：上拉
//返回值：0：成功
static uint8_t GpioSetInput(uint8_t lev){
	gpio_mode_t pullMode=GPIO_MODE_INPUT_PULL_DOWN;

	if(lev){
		pullMode=GPIO_MODE_INPUT_PULL_UP;
	}
	rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);
	rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOB, true);
	rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOD, true);

	gpio_set_iomux(GPIOA,GPIO_PIN_6,0);
	gpio_set_iomux(GPIOA,GPIO_PIN_7,0);
	
	gpio_init(GPIOA,GPIO_PIN_2, pullMode);	//IO2
	gpio_init(GPIOA,GPIO_PIN_4, pullMode);	//IO4
	gpio_init(GPIOA,GPIO_PIN_5, pullMode);	//IO5
	gpio_init(GPIOA,GPIO_PIN_6, pullMode);	//IO6
	gpio_init(GPIOA,GPIO_PIN_7, pullMode);	//IO7
	gpio_init(GPIOA,GPIO_PIN_8, pullMode);	//IO8
	gpio_init(GPIOA,GPIO_PIN_9, pullMode);	//IO9
	gpio_init(GPIOA,GPIO_PIN_11, pullMode);	//IO11
	gpio_init(GPIOA,GPIO_PIN_14, pullMode);	//IO14
	gpio_init(GPIOA,GPIO_PIN_15, pullMode);	//IO15
	gpio_init(GPIOD,GPIO_PIN_12, pullMode);	//IO60

	return 0;
}

//读取GPIO电平(注意：没有使用的和设置为输出模式的都会返回为0)
//参数是一个u8[8]的数组地址，用来接收返回的gpio的电平，这个是位操作，每一个位表示一个gpio
//    eg：bit0=0，bit11(buf[1]的bit4)=1表示IO0低电平，IO11为高电平
static void GpioGetInput(uint8_t *pBuf){
	memset(pBuf,0,8);
	pBuf[0]|=gpio_read(GPIOA,GPIO_PIN_2)<<2;	//IO2
	pBuf[0]|=gpio_read(GPIOA,GPIO_PIN_4)<<4;	//IO4
	pBuf[0]|=gpio_read(GPIOA,GPIO_PIN_5)<<5;	//IO5
	pBuf[0]|=gpio_read(GPIOA,GPIO_PIN_6)<<6;	//IO6
	pBuf[0]|=gpio_read(GPIOA,GPIO_PIN_7)<<7;	//IO7
	pBuf[1]|=gpio_read(GPIOA,GPIO_PIN_8)<<(8%8);	//IO8
	pBuf[1]|=gpio_read(GPIOA,GPIO_PIN_9)<<(9%8);	//IO9
	pBuf[1]|=gpio_read(GPIOA,GPIO_PIN_11)<<(11%8);	//IO11
	pBuf[1]|=gpio_read(GPIOA,GPIO_PIN_14)<<(14%8);	//IO14
	pBuf[1]|=gpio_read(GPIOA,GPIO_PIN_15)<<(15%8);	//IO15
}

//---------------------------------------- 事件处理函数 ---------------------------------------
//查询SF任务
static void ProcessQueryFreqEvent(void){
	static uint32_t queryFreqSysTickBak=0;
	static uint8_t currentQueryFreqIndex=0;	//当前查询频率在频率列表中的索引号
	uint32_t currentSfSystickTimeout=0;	//不同sf查询的超时时间
	
	if(g_simpleCmdConfig.isTestBoard){
		return;	//测试底板不执行该任务
	}
	
	switch(g_cmdStatus.eventQueryFreq){
		case EVENT_QUERY_FREQ_NOT_RUN:	//任务没有触发
			break;
		case EVENT_QUERY_FREQ_INIT:	//任务初始化
			currentQueryFreqIndex=0;
			printf("\r\nstart SF query task\r\nsRF config:\r\n    power:%d\r\n",g_cmdStatus.modulePower);
			switch(g_cmdStatus.moduleBw){
				case 0:
					printf("    bw:125k\r\n");
					break;
				case 1:
					printf("    bw:250k\r\n");
					break;
				case 2:
					printf("    bw:500k\r\n");
					break;
				default:
					printf("    bw:error(%d)\r\n",g_cmdStatus.moduleBw);
					break;
			}
			printf("    sf:%-62d*\r\n",g_cmdStatus.moduleSf);
			switch(g_cmdStatus.moduleCr){
				case 1:
					printf("    cr:[4/5]\r\n");
					break;
				case 2:
					printf("    cr:[4/6]\r\n");
					break;
				case 3:
					printf("    cr:[4/7]\r\n");
					break;
				case 4:
					printf("    cr:[4/8]\r\n");
					break;
				default:
					printf("    cr:error(%d),stop SF query task\r\n",g_cmdStatus.moduleCr);
					break;
			}
			printf("    freqOffset:%d\r\n",g_cmdStatus.moduleFreqOffset);
			g_cmdStatus.moduleFreq=g_simpleCmdConfig.testBoardStartFreq[currentQueryFreqIndex]+g_cmdStatus.moduleFreqOffset;
			//构建频率查询数据包
			g_cmdStatus.txBuf[0]=LORA_FACTORY_TEST_CMD_QUERY_FREQ;
			memcpy(g_cmdStatus.txBuf+1,g_cmdStatus.sn,8);
			g_cmdStatus.txBuf[9]=0x04;
			memcpy(g_cmdStatus.txBuf+10,&g_cmdStatus.moduleFreq,4);
			//设置收发参数
			Radio.SetChannel(g_cmdStatus.moduleFreq);
			Radio.SetRxConfig(MODEM_LORA,g_cmdStatus.moduleBw,g_cmdStatus.moduleSf,g_cmdStatus.moduleCr,0,LORA_PREAMBLE_LENGTH,
                      LORA_SYMBOL_TIMEOUT,LORA_FIX_LENGTH_PAYLOAD_ON,0,true,0,0,LORA_IQ_INVERSION_ON,true);
			Radio.SetTxConfig(MODEM_LORA,g_cmdStatus.modulePower,0,g_cmdStatus.moduleBw,g_cmdStatus.moduleSf,g_cmdStatus.moduleCr,
                      LORA_PREAMBLE_LENGTH,LORA_FIX_LENGTH_PAYLOAD_ON,true,0,0,LORA_IQ_INVERSION_ON,60000);

			printf("\r\n[QueryFreqEvent]start query freq=%d\r\n",g_cmdStatus.moduleFreq);
			queryFreqSysTickBak=g_sysTick;
			Radio.Standby();
			g_cmdStatus.eventQueryFreq=EVENT_QUERY_FREQ_WAIT_ACK;
			Radio.Send(g_cmdStatus.txBuf,14);	//发送查询包
			break;
		case EVENT_QUERY_FREQ_WAIT_ACK:	//等待SF查询响应
			//超时设置
			switch(g_cmdStatus.moduleSf){
				case 7:
					currentSfSystickTimeout=180;
					break;
				case 8:
					currentSfSystickTimeout=230;
					break;
				case 9:
					currentSfSystickTimeout=300;
					break;
				case 10:
					currentSfSystickTimeout=500;
					break;
				case 11:
					currentSfSystickTimeout=800;
					break;
				case 12:
					currentSfSystickTimeout=1500;
					break;
				default:
					printf("[%s()-%d]unknown SF=%d stop SF query task\r\n",__func__,__LINE__,g_cmdStatus.moduleSf);
					Radio.Standby();
					g_cmdStatus.eventQueryFreq=EVENT_QUERY_FREQ_NOT_RUN;
					break;
			}
			if(GetTimePassedMs(queryFreqSysTickBak) > currentSfSystickTimeout){
				printf("\r\n[QueryFreqEvent]query freq=%d timeout,start next freq\r\n",g_cmdStatus.moduleFreq);
				g_cmdStatus.eventQueryFreq=EVENT_QUERY_FREQ_NEXT_FREQ;
				break;
			}
			break;
		case EVENT_QUERY_FREQ_NEXT_FREQ:	//轮询下一个SF频道
			currentQueryFreqIndex++;
			if(currentQueryFreqIndex>=TEST_MACHINE_MAX_NUMBER){
				printf("[QueryFreqEvent]query freq end,stop task\r\n");
				Radio.Standby();
				g_cmdStatus.eventQueryFreq=EVENT_QUERY_FREQ_NOT_RUN;
				break;
			}
			g_cmdStatus.moduleFreq=g_simpleCmdConfig.testBoardStartFreq[currentQueryFreqIndex]+g_cmdStatus.moduleFreqOffset;
			//构建查询sf数据包
			g_cmdStatus.txBuf[0]=LORA_FACTORY_TEST_CMD_QUERY_FREQ;
			memcpy(g_cmdStatus.txBuf+1,g_cmdStatus.sn,8);
			g_cmdStatus.txBuf[9]=0x04;
			memcpy(g_cmdStatus.txBuf+10,&g_cmdStatus.moduleFreq,4);
			//设置收发参数
			Radio.SetChannel(g_cmdStatus.moduleFreq);
			Radio.Init(&g_FactoryTestRadioEvents);
			Radio.SetRxConfig(MODEM_LORA,g_cmdStatus.moduleBw,g_cmdStatus.moduleSf,g_cmdStatus.moduleCr,0,LORA_PREAMBLE_LENGTH,
                      LORA_SYMBOL_TIMEOUT,LORA_FIX_LENGTH_PAYLOAD_ON,0,true,0,0,LORA_IQ_INVERSION_ON,true);
			Radio.SetTxConfig(MODEM_LORA,g_cmdStatus.modulePower,0,g_cmdStatus.moduleBw,g_cmdStatus.moduleSf,g_cmdStatus.moduleCr,
                      LORA_PREAMBLE_LENGTH,LORA_FIX_LENGTH_PAYLOAD_ON,true,0,0,LORA_IQ_INVERSION_ON,60000);
			
			printf("\r\n[QueryFreqEvent]start query freq=%d\r\n",g_cmdStatus.moduleFreq);
			queryFreqSysTickBak=g_sysTick;
			Radio.Standby();
			g_cmdStatus.eventQueryFreq=EVENT_QUERY_FREQ_WAIT_ACK;
			Radio.Send(g_cmdStatus.txBuf,14);	//发送查询包
			break;
		default:
			printf("[%s()-%d]eventQuerySf=%d is error\r\n",__func__,__LINE__,g_cmdStatus.eventQueryFreq);
			Radio.Standby();
			g_cmdStatus.eventQueryFreq=EVENT_QUERY_FREQ_NOT_RUN;
			break;
	}
}

//设置测试底板GPIO任务
static void ProcessSetGpioEvent(void){
	static uint32_t queryFreqSysTickBak=0;
	uint32_t currentSfSystickTimeout=0;	//不同sf查询的超时时间
	
	if(g_simpleCmdConfig.isTestBoard){
		return;	//测试底板不执行该任务
	}
	
	switch(g_cmdStatus.eventSetGpio){
		case EVENT_SET_GPIO_NOT_RUN:	//任务没有触发
			queryFreqSysTickBak=g_sysTick;
			break;
		case EVENT_SET_GPIO_WAIT_ACK:	//等待GPIO设置指令响应
			//超时设置
			switch(g_cmdStatus.moduleSf){
				case 7:
					currentSfSystickTimeout=180;
					break;
				case 8:
					currentSfSystickTimeout=230;
					break;
				case 9:
					currentSfSystickTimeout=300;
					break;
				case 10:
					currentSfSystickTimeout=500;
					break;
				case 11:
					currentSfSystickTimeout=800;
					break;
				case 12:
					currentSfSystickTimeout=1500;
					break;
				default:
					printf("[%s()-%d]unknown SF=%d stop SF query task\r\n",__func__,__LINE__,g_cmdStatus.moduleSf);
					g_cmdStatus.eventSetGpio=EVENT_SET_GPIO_NOT_RUN;
					break;
			}
			if(GetTimePassedMs(queryFreqSysTickBak) > currentSfSystickTimeout){
				printf("\r\n[SetGPIOEvent]wait GPIO set ack timeout,stop wait\r\n");
				Radio.Standby();
				g_cmdStatus.eventSetGpio=EVENT_SET_GPIO_NOT_RUN;
				break;
			}
			break;
		default:
			printf("[%s()-%d]eventSetGpio=%d is error\r\n",__func__,__LINE__,g_cmdStatus.eventSetGpio);
			Radio.Standby();
			g_cmdStatus.eventSetGpio=EVENT_SET_GPIO_NOT_RUN;
			break;
	}
}

