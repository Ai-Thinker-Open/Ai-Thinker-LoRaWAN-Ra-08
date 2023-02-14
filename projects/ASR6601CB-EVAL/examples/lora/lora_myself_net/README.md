# Polling Calling Example

## Introduction

This example shows how to use the Ra-08 to accomplish polling function.

## How to use example

Open the project, comment or uncomment **'#define CONFIG_GATEWAY (1)'** code on line 19 in **‘projects/ASR6601CB-EVAL/examples/lora/lora_myself_net/components/lora_driver/include/lora_config.h’** to set module as Gateway or Node.
For example : 

```
#define CONFIG_GATEWAY (1)	//to set module as Gateway
//#define CONFIG_GATEWAY (1)	//to set module as node
```

Finally,running command below to compile project on ubuntu.
```
make -j32
```

Notice! You should run **'source build/envsetup.sh'** on root path first when you got **'there is no rule ...'** error.


## Burn firmware

burn firmware through **“TremoProgrammer_v0.8.exe”** tool.

First, connect the serial port 0 of the module to the serial port of the PC, and secondly, pull up the IO2 level of the module, and then reset it to make the module enter the burning mode; Finally, click **“ERASE ALL”** to erase the firmware, and then click **“START”** to burn the firmware;

## Debug

Open the serial port debugging assistant software,select the COM port of the development board,and press the reset button on development board to run the program finally.



1. Gateway firmware
Polling_Gateway.bin
Instructions:
1.1. Command to set the number of patrolling nodes:
						A0 F2 1E 01 11 22 85 A1
		Command description:
		A0 is the header; A1 is the end of the packet;
		F2 is the command type, and F2 is the data setting of the tour;
		1E is the number of nodes, 0x1E is 30;
		85 is the checksum;
		The rest are reserved bits (currently have no function)

​		1.2. Log analysis:
​			The node reply timeout will display: ADDR x get ack timeout (x indicates the address of the device)
​			When receiving a node reply, it will reply with the following content:
​			Get data from: 1 , set next call node: 2, polling nodes num: 2

1111111111112222222222222222233333333333333333333333444444444444444444444555555555555555555555555666666666666666666666667777777777777777777777777777788888888888888888888999999999999999000000000000000

​			Among them, Get data from: 1 indicates that the data from address 1 is received;

​			Indicates that the data sent by the node is received, and that two hundred data are fixed.



2. Node firmware 

   Polling_Node.bin 

   Instructions: 

   2.1. Set node address command:

   ​		A0 1E 00 00 00 00 A1 

   ​		A0 is the header; 

   ​		A1 is the end of the packet; 

   ​		1E is the address number, the address is unsigned 8-bit data, and the configurable range is 0~255; 

   2.2. Log analysis: When the node receives the request from the Gateway, it will reply to receive the data and print the detailed content of the data:

​			Recieve New Msg , length:[8] 
​			Recieve SLAVE1_ADDR: 1, receive addr: 1 data: A0 F1 01 02 11 22 33 A1

## Troubleshooting

For any technical queries, please open an [issue](https://github.com/Ai-Thinker-Open/Ai-Thinker-WB2/issues) on GitHub. We will get back to you soon.



----------------------------------


# 轮询呼叫例程

## 简介

本例程主要介绍使用Ra-08模组实现轮询功能。

## 如何使用该例程

打开项目，将文件 **”projects/ASR6601CB-EVAL/examples/lora/lora_myself_net/components/lora_driver/include/lora_config.h“** 中的 **”#define CONFIG_GATEWAY (1)“** 代码给注释掉或者去注释的方式配置模组为节点还是网关；
如下：

```
#define CONFIG_GATEWAY (1)	//设置模组为网关
//#define CONFIG_GATEWAY (1)	//设置模组为节点
```
最后，在ubuntu下运行以下指令编译工程。
```
make -j32
```

注意：若是在编译过程中遇到 **”Make 没有规则......“** 的错误提示的时候，回到根目录下运行指令 **”source build/envsetup.sh“** 。

## 烧录固件

使用 **”TremoProgrammer_v0.8.exe“** 工具烧录固件。

首先，将模组的串口0与PC机的串口连接，其次，要将模组的IO2电平拉高，之后复位一下，使模组进入烧录模式；

最后，点击 **“ERASE ALL”** 擦除固件，再点击 **“START”** 烧录固件；




## 调试

打开“串口调试助手软件”，选择并打开开发板的COM口，最后按下复位键启动程序。

1、网关固件
Polling_Gateway.bin
使用方法：
	1.1、设置轮巡节点数量指令：
		A0 F2 1E 01 11 22 85 A1
		指令说明：
			A0为包头；A1为包尾；
			F2为指令类型，F2为轮巡个数据设置；
			1E为节点数量，0x1E即30个；
			85为校验和；
			其余为保留位（目前无功能）

1.2、Log分析：
	节点回复超时将显示：ADDR x get ack timeout（x表示设备的地址）
	当收到节点回复时将回复一下内容：
	Get data from: 1 ,set next call node: 2, polling nodes num: 2 
	data: 1111111111112222222222222222233333333333333333333333444444444444444444444555555555555555555555555666666666666666666666667777777777777777777777777777788888888888888888888999999999999999000000000000000
	其中Get data from: 1表示收到来自地址1的数据；
	data：1111111111112222222222222222233333333333333333333333444444444444444444444555555555555555555555555666666666666666666666667777777777777777777777777777788888888888888888888999999999999999000000000000000
	表示收到节点发过来的数据，这两百个数据是固定的



2、节点固件
Polling_Node.bin
使用方法：
	2.1、设置节点地址指令
	A0 1E 00 00 00 00 A1
		A0为包头；A1为包尾；
		1E为地址编号，地址为无符号8位数据，可配置范围是0~255；
	2.2、Log分析：
	当节点收到Gateway的请求的时候，将回复接收到数据，并打印数据详细内容：
	Recieve New Msg , length:[8] 
	Recieve SLAVE1_ADDR: 1, receive addr: 1 data: A0 F1 01 02 11 22 33 A1
	

## 问题排除

若有任何问题，请在github中提交一个[issue](https://github.com/Ai-Thinker-Open/Ai-Thinker-WB2/issues)，我们会尽快回复。
