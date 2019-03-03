#!/bin/bash
BIN=bin
PROG=src/vlt

if [ ! -d "$BIN" ]; then
  mkdir $BIN
fi

#make clean
#make
cp -f $PROG $BIN
cd $BIN && ./vlt -t 'Hello, World!' -u http://0.0.0.0:4545 -a -f ../assets/sample.mp4 -l 1
