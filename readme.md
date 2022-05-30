# 代码说明

​		该代码是ASR6601sdk，主要更新的是AT指令demo

​		该SDK同步修改的工程包括一下几个

​		产测使用的工程 projects/ASR6601CB-EVAL/examples/lora/lora_factory_test/

​		定频测试使用的 projects/ASR6601CB-EVAL/examples/lora/lora_test/

​		OTA升级发射端程序 projects/ASR6601CB-EVAL/examples/ota/dongle/

​		OTA升级使用的bootloader工程 projects/ASR6601CB-EVAL/examples/ota/bootloader/

​		LoRaWAN AT指令工程 projects/ASR6601CB-EVAL/examples/lorawan/lorawan_at/

# 编译AT固件方法

​		编译不同频段的固件可以执行根目录下的 make_lorawan_at_xxx.sh

​		编译生成的固件路径为 projects/ASR6601CB-EVAL/examples/lorawan/lorawan_at/out/lorawan_at.bin(编译完成后固件会单独拷贝一份到SDK根目录下的out目录)

# 编译dongle固件方法

​		执行SDK根目录下的 make_lora_test_and_lora_factory_test.sh

​		固件会拷贝到sdk根目录下的out目录

# 修改文件

​		本SDK针对原始SDK修改主要在projects/ASR6601CB-EVAL/examples/lorawan/lorawan_at/目录下的文件，同时在根目录添加了几个编译脚本
