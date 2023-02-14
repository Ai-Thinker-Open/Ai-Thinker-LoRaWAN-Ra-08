# 安信可LoRaWAN模组Ra-08(H)二次开发入门指南

* [English](./readme.md)

# 目录

# <span id = "Introduction">0.介绍</span>
[安信可](https://www.espressif.com/zh-hans)是物联网无线的设计专家，专注于设计简单灵活、易于制造和部署的解决方案。安信可研发和设计 IoT 业内集成度SoC、性能稳定、功耗低的无线系统级模组产品，有 Wi-Fi 、LoRaWAN、蓝牙、UWB 功能的各类模组，模组具备出色的射频性能。

Ra-08(H) 模组是安信可科技与上海翱捷科技（**简称ASR**）深度合作、共同研发的一款 LoRaWAN 模组，本仓库是对应此LoRaWAN模组 SoC 二次开发入门指南，模组对应的芯片型号为**ASR6601CB、 Flash 128 KB 、SRAM  16 KB、内核 32-bit 48 MHz ARM Cortex-M4**。

Ra-08(H) 模组出厂内置 AT 固件程序，直接上手使用对接LoRaWAN网关。如需对接 阿里LinkWAN 需编程本仓库代码。

# <span id = "aim">1.目的</span>
本文基于 linux 环境，介绍安信可 Ra-08(H) 模组二次开发点对点通讯的具体流程，供读者参考。

# <span id = "hardwareprepare">2.硬件准备</span>
- **linux 环境**  
用来编译 & 烧写 & 运行等操作的必须环境，本文以 （Ubuntu18.04） 为例。 
> windows 用户可安装虚拟机，在虚拟机中安装 linux。

- **设备**  
前往安信可官方获取 2 PCS：[样品](https://anxinke.taobao.com) ，带小辣椒天线。

- **USB 线**  
连接 PC 和 Ra-08 开发板，用来烧写/下载程序，查看 log 等。

# <span id = "aliyunprepare">3.Ra-08开发板准备</span>

| 安信可在售模组 | 是否支持 |
| -------------- | -------- |
| Ra-08          | 支持     |
| Ra-08H         | 支持     |

# <span id = "compileprepare">4.编译器环境搭建</span>
```
sudo apt-get install gcc-arm-none-eabi git vim python python-pip
pip install pyserial configparser 
```

# <span id = "sdkprepare">5.SDK 准备</span> 

```
git clone --recursive https://github.com/Ai-Thinker-Open/Ai-Thinker-LoRaWAN-Ra-08.git
```


# <span id = "makeflash">6.编译 & 烧写 & 运行</span>
## 6.1 编译

### 6.1.1 配置环境变量

 ```
 source build/envsetup.sh
 ```

### 6.1.2 编译点对点通讯的示例
```
cd projects/ASR6601CB-EVAL/examples/lora/pingpong/
make
```

编译成功后，出现这样的LOG

```
"arm-none-eabi-size" out/pingpong.elf
   text    data     bss     dec     hex filename
  21312    1092    4656   27060    69b4 out/pingpong.elf
Please run 'make flash' or the following command to download the app
python /mnt/d/GitHub/ASR6601_AT_LoRaWAN/build/scripts/tremo_loader.py -p /dev/ttyUSB0 -b 921600 flash 0x08000000 out/pingpong.bin
```

找到烧写串口，然后开始烧录，比如我的接入串口是 /dev/ttyUSB2 ：

```
python /mnt/d/GitHub/ASR6601_AT_LoRaWAN/build/scripts/tremo_loader.py -p /dev/ttyUSB2 -b 921600 flash 0x08000000 out/pingpong.bin
```

## 6.2 清空工程编译文件 & 编译烧写 & 下载固件 & 查看 log
将 USB 线连接好设备和 PC，确保烧写端口正确，并且按照下面操作把模块进去下载模式。


<p align="center">
  <img src="png\connect.png" width="480px" height="370px" alt="Banner" />
</p>


先修改默认的连接硬件的端口号、波特率，在文件 ```\build\make\common.mk``` 里面修改：

```
# flash settings
TREMO_LOADER := $(SCRIPTS_PATH)/tremo_loader.py
SERIAL_PORT        ?= /dev/ttyUSB0
SERIAL_BAUDRATE    ?= 921600
```

### 6.2.1 清空工程编译文件

```
make clean
```
> 注：无需每次擦除，如修改配置文件重新编译，需执行此操作。

### 6.2.2 烧录程序
```
make flash
```

### 6.2.3 运行

2个 Ra-08-Kit 开发板按下 RST 按键，即可看到如下 log：

```
Received: PING
Sent: PONG
Received: PING
Sent: PONG
Received: PING
Sent: PONG
Received: PING
Sent: PONG
Received: PING
Sent: PONG
Received: PING
Sent: PONG
```
