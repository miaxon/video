#!/bin/bash
BIN=bin
PROG=src/vlt

if [ ! -d "$BIN" ]; then
  mkdir $BIN
fi

#make clean
#make
cp -f $PROG $BIN
cd $BIN && ./vlt -t 'Hello, World! Я здесь.' -u http://127.0.0.1:4545
