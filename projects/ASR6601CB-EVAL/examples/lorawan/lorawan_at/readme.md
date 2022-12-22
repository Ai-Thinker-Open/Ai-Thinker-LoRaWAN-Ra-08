# Extra LoraWAN AT building information
The AT firmware as presented in this SDK requires the OTA firmware installed, which has to be either installed by flashing full original LORAWAN AT firmware first, or by building it in [../../ota/bootloader/](../../ota/bootloader/)  directory and flashing it before installing the custom-built AT firmware.

To build AT firmware for your specific LoRaWAN band, follow the instructions in main document [/readme.md](/readme.md), especially

```
source build/envsetup.sh
```

Before building the specific variant of the firmware, use one of following lines to export the correct variable (not needed for CN470 band):

```
export LoraWanRegionDefine=REGION_EU868 # for Ra-08H EU	
export LoraWanRegionDefine=REGION_US915 # for Ra-08H US band	
export LoraWanRegionDefine=REGION_AU915 # for Ra-08H Australia band
export LoraWanRegionDefine=REGION_EU433 # for Ra08 EU 433 band
export LoraWanRegionDefine=REGION_CN470 # for Ra08 CN 470 band (default if this variable is not set)
...
```
After that, you can continue with 

```
make clean
make
make flash # do not forget to put the board in download mode first
```

You can add any future aditional regions by modifying the *Makefile* and *inc/aithinker_code.h* to support them. The additional available bands can be found in [/lora/mac/region](/lora/mac/region/) directory. The modifications would look like this:

```
else ifeq ($(LoraWanRegionDefine),REGION_XXFFF)
        $(PROJECT)_SOURCE += $(TREMO_SDK_PATH)/lora/mac/region/RegionXXFFF.c
        $(PROJECT)_DEFINES := -DCONFIG_DEBUG_UART=UART0 -DREGION_XXFFF -DCONFIG_LWAN -DCONFIG_LWAN_AT -DCONFIG_LOG -DPRINT_BY_DMA
        
```
in Makefile before final else, and in *inc/aithinker_code.h* 
```
#elif defined(REGION_XXFFF)
       #define LORAWAN_REGION_STR      "XXFFF"
```
where *XX* is the country code and *FFF* is the frequency of band in MHz.
