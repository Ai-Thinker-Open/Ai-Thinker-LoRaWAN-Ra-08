#/bin/sh

export TREMO_SDK_PATH=$(pwd)

which arm-none-eabi-gcc >/dev/null 2>&1
if [ $? -ne 0 ]; then
    if [ ! -e $TREMO_SDK_PATH/tools/toolchain/bin/arm-none-eabi-gcc.exe ]; then
    	cd ./tools/toolchain/
    	unzip gcc-arm-none-eabi-9-2020-q2-update-win32.zip
    	cd -
    fi
    
    which arm-none-eabi-gcc >/dev/null 2>&1
    if [ $? -ne 0 ]; then
        export PATH="$PATH:$TREMO_SDK_PATH/tools/toolchain/bin/"
    fi
fi
