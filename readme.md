# Ai-Thinker LoRaWAN Module Ra-08(H) Secondary Development Getting Started Guide

* [中文版](./readme_cn.md)

# Content

# <span id = "Introduction">0.Introduction</span>
[Ai-Thinker](https://docs.ai-thinker.com/welcome) is an expert in IoT wireless design, focusing on solutions that are simple, flexible, easy to manufacture and deploy. [Ai-Thinker](https://docs.ai-thinker.com/welcome) develops and designs wireless system-level module products with integrated SoC, stable performance, and low power consumption in the IoT industry, and various modules with Wi-Fi, LoRaWAN, Bluetooth, and UWB functions. The modules have excellent RF performance.

Ra-08(H) module is a LoRaWAN module jointly developed by [Ai-Thinker](https://docs.ai-thinker.com/welcome) Technology and Shanghai ASR Microelectronics Co., Ltd.(ASR) in depth. This repository is a guide for getting started with the secondary development of the LoRaWAN module SoC. The corresponding chip model is ASR6601CB, Flash 128 KB, SRAM 16 KB, 32-bit 48 MHz ARM Cortex-M4 kernel.

The Ra-08(H) module has a built-in AT firmware program default, and it can be used directly to connect to the LoRaWAN gateway. If you need to connect to Ali LinkWAN, you need to program this repository code.

# <span id = "aim">1.Purpose</span>
Based on the linux environment, this article introduces the specific process of secondary development of point-to-point communication of An-Thinker’s Ra-08(H) module for your reference. 

# <span id = "hardwareprepare">2.Hardware preparation</span>
- **•	Linux environment**  
The necessary environment for compiling & programming & running operations, this article takes (Ubuntu18.04) as an example.
> (Windows users can install a virtual machine and then install linux in the virtual machine.)

- **Equipment**  
Get samples from Ai-Thinker’s [Alibaba store](https://anxinke.taobao.com) to get 2 pcs module and the antenna as well. 


- **USB cable**  
Connect PC and Ra-08 development board to burn/download programs, view logs, etc.

# <span id = "aliyunprepare">3.Ra-08 development board preparation</span>

| On sale module | Whether to support |
| -------------- | ------------------ |
| Ra-08          | support            |
| Ra-08H         | support            |

# <span id = "compileprepare">4.Build the compiler environment</span>
```
sudo apt-get install gcc-arm-none-eabi git vim python python-pip
pip install pyserial configparser 
```

# <span id = "sdkprepare">5.SDK preparation</span> 

```
git clone --recursive https://github.com/Ai-Thinker-Open/Ai-Thinker-LoRaWAN-Ra-08.git
```

# <span id = "makeflash">6.Compiling & Programming & Running</span>

## 6.1 Compiling

### 6.1.1 Configure environment variables

 ```
 source build/envsetup.sh
 ```

### 6.1.2 Example of compiling peer-to-peer communication
```
cd projects/ASR6601CB-EVAL/examples/lora/pingpong/
make
```

Example of compiling peer-to-peer communication

```
"arm-none-eabi-size" out/pingpong.elf
   text    data     bss     dec     hex filename
  21312    1092    4656   27060    69b4 out/pingpong.elf
Please run 'make flash' or the following command to download the app
python /mnt/d/GitHub/ASR6601_AT_LoRaWAN/build/scripts/tremo_loader.py -p /dev/ttyUSB0 -b 921600 flash 0x08000000 out/pingpong.bin
```

Find the burning serial port, and then start burning, for example, my access serial port here is /dev/ttyUSB2 ：

```
python /mnt/d/GitHub/ASR6601_AT_LoRaWAN/build/scripts/tremo_loader.py -p /dev/ttyUSB2 -b 921600 flash 0x08000000 out/pingpong.bin
```

## 6.2 Clear project compilation files & compile & flash & download firmware & view log

Connect the USB cable to the device and PC, make sure the programming port is correct, and follow the steps below to put the module into the download mode


<p align="center">
  <img src="png\connect_en.png" width="480px" height="370px" alt="Banner" />
</p>


First modify the default port number and baud rate to connect to the hardware, and modify it in the file ```\build\make\common.mk``` ：

```
# flash settings
TREMO_LOADER := $(SCRIPTS_PATH)/tremo_loader.py
SERIAL_PORT        ?= /dev/ttyUSB0
SERIAL_BAUDRATE    ?= 921600
```

### 6.2.1 Clear the project compilation file

```
make clean
```
>Note: It is not necessary to erase every time, if you modify the configuration file and recompile, you need to perform this operation.

### 6.2.2 Burning program
```
make flash
```

### 6.2.3 Running

Press the RST button on two Ra-08-Kit development boards, and you can see the following log:

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
