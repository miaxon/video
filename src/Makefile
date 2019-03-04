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
LIB_AVLDFLAGS  := $(shell pkg-config --libs  $(FFMPEG_LIBS))
LIB_AVCFLAGS   := $(shell pkg-config --cflags $(FFMPEG_LIBS))
LIB_ASSLDFLAGS := $(shell pkg-config --libs libass)

.PHONY: all clean

all: $(prog)

$(prog): $(obj)
	$(CC) -o $@ $^ $(LINKFLAGS) $(LDFLAGS) $(LIB_AVLDFLAGS) $(LIB_ASSLDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS) $(GIT_FLAGS) $(inc_flags) $(LIB_AVCFLAGS) -MMD

include $(wildcard *.d)

clean: 
	rm -f $(prog)
	rm -f *.o
	rm -f *.d
