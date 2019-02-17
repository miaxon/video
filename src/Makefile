# Makefile builds Control Client
#
include ../Makefile.inc

prog := vlt

FFMPEG_LIBS=    libavdevice                        \
                libavformat                        \
                libavfilter                        \
                libavcodec                         \
                libswresample                      \
                libswscale                         \
                libavutil                          \

#LDFLAGS += -lavformat -lavcodec -lswscale -lavutil -lavfilter -lswresample -lavdevice -pthread -lfdk-aac -lvorbis -lmp3lame -lx264 -lrt -lm -lz -lrt -lX11 -llzma -lbz2 -lvdpau
LIB_LDFLAGS := $(shell pkg-config --libs $(FFMPEG_LIBS))
LIB_CFLAGS := $(shell pkg-config --cflags $(FFMPEG_LIBS))
.PHONY: all clean

all: $(prog)

$(prog): $(obj)
	$(CC) -o $@ $^ $(LINKFLAGS) $(LDFLAGS) $(LIB_LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS) $(GIT_FLAGS) $(inc_flags) $(LIB_CFLAGS) -MMD

include $(wildcard *.d)

clean: 
	rm -f $(prog)
	rm -f *.o
	rm -f *.d
