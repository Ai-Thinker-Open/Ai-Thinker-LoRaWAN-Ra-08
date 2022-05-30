#ifndef __SIMPLE_CMD_H__
#define __SIMPLE_CMD_H__

#define DEBUG_SPECTER	0	//调试模式
//产测固件版本
#define FACTORY_TEST_VERSION	"v0.0.4"
//#define FACTORY_TEST_VERSION	"v0.0.4-debug"

//配置选项
#define SIMPLE_CMD_RX_BUF_SIZE	(256)	//串口buf大小
#define SIMPLE_CMD_CONFIG_FLASH_SIZE	(4096)	//flash分区给配置文件预留的分区
#define SIMPLE_CMD_CONFIG_FLASH_ADDR	(0x0801F000)	//flash保存的首地址(cb的flash为128k的，最后4k作为配置保存地址)
#define TEST_MACHINE_MAX_NUMBER	(5)	//最多可以记录几个治具的起始频率(该频率用于轮询频率时使用)

//事件定义
#define EVENT_CMD_START	(0x00000001)	//开始解析指令

//状态机定义
//通信频率查询任务事件
typedef enum{
	EVENT_QUERY_FREQ_NOT_RUN=0,	//任务没有触发
	EVENT_QUERY_FREQ_INIT,	//任务初始化
	EVENT_QUERY_FREQ_WAIT_ACK,	//等待频率查询响应
	EVENT_QUERY_FREQ_NEXT_FREQ,	//轮询下一个频率
}EventQueryFreq;

//GPIO设置任务事件
typedef enum{
	EVENT_SET_GPIO_NOT_RUN=0,	//任务没有触发
	EVENT_SET_GPIO_WAIT_ACK,	//等待GPIO设置响应
}EventSetGpio;

//数据类型定义
typedef struct{
	volatile uint32_t eventFlag;	//事件标志位
	//串口相关
	volatile int	reciveCount;	//串口接收到的数据长度
	volatile uint8_t	reciveBuf[SIMPLE_CMD_RX_BUF_SIZE];	//串口接收buf
	uint8_t	sn[8];
	uint8_t	txBuf[256];	//发送buf(这个是公用的，自己注意互斥使用)
	//事件相关
	EventQueryFreq	eventQueryFreq;	//频率查询事件状态机(0表示该任务没有触发)
	EventSetGpio	eventSetGpio;	//设置GPIO事件
	//待测模组射频参数
	uint8_t	modulePower;	//功率2~22dbm
	uint8_t	moduleSf;	//扩频因子[SF7~SF12]
	uint8_t	moduleBw;	//带宽[0: 125 kHz,	1: 250 kHz,	2: 500 kHz,	3: Reserved]
	uint8_t	moduleCr;	//纠错编码率[1: 4/5,	2: 4/6,	3: 4/7,	4: 4/8]
	int	moduleFreq;	//频率
	int moduleFreqOffset;	//测试频率偏移值
}SimpleCmd_s;

//保存配置结构体
typedef struct{
	uint8_t isTestBoard:1;	//当前程序是否是测试底板，1：是测试底板；0：不是测试底板(不是底板就是待测模组)
	//lora相关
	uint8_t testBoardPower;	//功率2~22dbm
	int ackRssiOffset;	//回复消息时携带的rssi偏移值，这个是为了调整不同工位的公差设计的(例如本工位模组发送过来底板接收到的rssi比其他的高2，那么这个就设置为-2，返回时就是在真实值上-2；注意：如果是底板发送给模组的有差异则调整底板的发射功率，而不需要调整这个offset)
	uint8_t testBoardSf;	//扩频因子[SF7~SF12]
	uint8_t testBoardBw;	//带宽[0: 125 kHz,	1: 250 kHz,	2: 500 kHz,	3: Reserved]
	uint8_t testBoardCr;	//纠错编码率[1: 4/5,	2: 4/6,	3: 4/7,	4: 4/8]
	int testBoardFreq;	//频率
	int testBoardStartFreq[TEST_MACHINE_MAX_NUMBER];	//存储治具起始测试频率的数组(这里只指定了治具第一个工位的模组的频率)
	//保存标志位，这个保存在最后未知，如果这个是 0x55表示已经存储了，如果不是则需要加载默认值
    uint8_t saveFlag;
}SimpleCmdConfig;

//变量导出和声明
extern SimpleCmd_s g_cmdStatus;
extern SimpleCmdConfig g_simpleCmdConfig;
extern volatile uint32_t g_sysTick;

void SimpleCmdStart(void);
uint8_t SimpleCmdLoadConfig(void);
uint8_t SimpleCmdSaveConfig(void);
uint32_t GetTimePassedMs(uint32_t bak);

#endif	//end of __SIMPLE_CMD_H__
