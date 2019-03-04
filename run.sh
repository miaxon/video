#!/bin/bash
BIN=bin
PROG=src/vlt

if [ ! -d "$BIN" ]; then
  mkdir $BIN
fi

#make clean
#make
cp -f $PROG $BIN
cd $BIN && ./vlt -t 'Hello, World!' -u http://0.0.0.0:4545 -a -f ../assets/sample.mp4 -l 1 -s udp://127.0.0.1:1234?pkt_size=1316
