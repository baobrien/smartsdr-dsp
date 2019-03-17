#!/bin/sh


if [ "$#" -ne 3 ]; then
  echo "usage: $0 <waveform name> <path to binary> <path to config>"
  exit
fi

WF_NAME=$1
WF_BIN=$2
WF_CFG=$3

mkdir ./$WF_NAME
cp ./$WF_CFG $WF_NAME/$WF_NAME.cfg
cp ./$WF_BIN $WF_NAME/

zip -r ./$WF_NAME.ssdr_waveform ./$WF_NAME 
