# Makefile builds Control Client
#
include ../Makefile.inc

prog := vlt

LDFLAGS += -lavformat -lavcodec -lswscale -lavutil -lavfilter -lswresample -lavdevice -pthread -lm -lz -lrt


.PHONY: all clean

debug: CFLAGS := $(DBGFLAGS)
debug: all

all: $(prog)

$(prog): $(obj)
	$(CC) -o $@ $^ $(LINKFLAGS) $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS) $(GIT_FLAGS) $(inc_flags) -MMD

include $(wildcard *.d)

clean: 
	rm -f $(prog)
	rm -f *.o
	rm -f *.d
