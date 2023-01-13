#/bin/sh

export TREMO_SDK_PATH=$(pwd)

GNU_ARM_EMBEDDED_TOOLCHAIN_URL='https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2020q2/gcc-arm-none-eabi-9-2020-q2-update-win32.zip?revision=95631fd0-0c29-41f4-8d0c-3702650bdd74&hash=60FBF84A2ADC7B1F508C2D625E831E1F1184F509'

which arm-none-eabi-gcc >/dev/null 2>&1
if [ $? -ne 0 ]; then
    if [ ! -e $TREMO_SDK_PATH/tools/toolchain/bin/arm-none-eabi-gcc.exe ]; then
        if [ ! -e $TREMO_SDK_PATH/tools/toolchain/gcc-arm-none-eabi-9-2020-q2-update-win32.zip ]; then
            wget $GNU_ARM_EMBEDDED_TOOLCHAIN_URL -O $TREMO_SDK_PATH/tools/toolchain/gcc-arm-none-eabi-9-2020-q2-update-win32.zip
        fi
        cd ./tools/toolchain/
        unzip gcc-arm-none-eabi-9-2020-q2-update-win32.zip
        cd -
    fi

    which arm-none-eabi-gcc >/dev/null 2>&1
    if [ $? -ne 0 ]; then
        export PATH="$PATH:$TREMO_SDK_PATH/tools/toolchain/bin/"
    fi
fi
