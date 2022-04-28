#!/bin/bash

ARCH="armhf"
while getopts "ha:" opt; do
    case $opt in
	a)
	    ARCH=$OPTARG
            ;;
        h)
            echo "Usage: create_kit.sh -a [arch]"
            echo -e "\t-a : Creat Kit for the given arch type.  Default: ${ARCH} [armhf, i386, amd64]"
	    exit
            ;;
       :)
           echo "Option -$OPTARG requires an argument." >&2
           exit 1
           ;;
    esac
done

QT_UID="UbuntuSDK-API-Qt"
SDK_TOOL="/usr/ubuntu-sdk-ide/bin/sdktool"
SDKPATH="./click-${ARCH}/usr/ubuntu-sdk-ide/share/qtcreator/QtProject/qtcreator"


rm ${SDKPATH}/*.xml

case "${ARCH}" in
  amd64)
    GCC_UID="ProjectExplorer.ToolChain.Gcc:UbuntuSDKGCC-x86_64-linux-gnu"
    NAME="GCC (x86 64bit in /opt/click.ubuntu.com/com.ubuntu.sdk/current/usr/bin/g++)"
    ABI="x86-linux-generic-elf-64bit" 
    SUPPORTED_ABIS="x86-linux-generic-elf-64bit,x86-linux-generic-elf-32bit"
    ;;

  i386)
    GCC_UID="ProjectExplorer.ToolChain.Gcc:UbuntuSDKGCC-x86-linux-generic-elf-32bit"
    NAME="GCC (x86 32bit in /opt/click.ubuntu.com/com.ubuntu.sdk/current/usr/bin/g++)"
    ABI="x86-linux-generic-elf-32bit"
    SUPPORTED_ABIS="x86-linux-generic-elf-32bit"
    ;;

  armhf)
    GCC_UID="ProjectExplorer.ToolChain.Gcc:UbuntuSDKGCC-arm-linux-generic-elf-32bit"
    NAME="GCC (armhf in /opt/click.ubuntu.com/com.ubuntu.sdk/current/usr/bin/g++)"
    ABI="arm-linux-generic-elf-32bit"
    SUPPORTED_ABIS="arm-linux-generic-elf-32bit"
    ;;

  *)
    echo "Unknown architecture"
    exit 1
    ;;
esac

${SDK_TOOL} --sdkpath=${SDKPATH} addTC \
      --id "${GCC_UID}" \
      --name "${NAME}" \
      --path /opt/click.ubuntu.com/com.ubuntu.sdk/current/usr/bin/g++ \
      --abi "${ABI}" \
      --supportedAbis "${SUPPORTED_ABIS}"

${SDK_TOOL} --sdkpath=${SDKPATH} addQt \
    --id   "${QT_UID}" \
    --name "Ubuntu SDK Desktop Qt" \
    --qmake /opt/click.ubuntu.com/com.ubuntu.sdk/current/usr/bin/qmake-clicksdk \
    --type Qt4ProjectManager.QtVersion.Desktop

${SDK_TOOL} --sdkpath=${SDKPATH} addKit \
        --id "UbuntuSDK-API-Desktop-Kit" \
        --name "Ubuntu SDK Desktop Kit" \
        --debuggerengine 1 \
        --debugger /opt/click.ubuntu.com/com.ubuntu.sdk/current/usr/bin/gdb \
        --devicetype Desktop \
        --toolchain "${GCC_UID}" \
        --qt "${QT_UID}"
