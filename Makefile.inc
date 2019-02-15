# Makefile.inc - common definitions used by all makefiles


src         := $(wildcard *.c)
obj         := $(src:.c=.o)
inc         := ../include
inc_flags   := -I$(inc)

LINKFLAGS   := 
LDFLAGS     := 

CFLAGS   += -std=gnu11 -Wall -Wpedantic  
DBGFLAGS := $(CFLAGS) -g -DDEBUG
CFLAGS   := $(CFLAGS) -O2 -fPIC

GIT_COMMIT := $(shell git describe --abbrev=8 --dirty --always --tags)
GIT_DATE   := $(wordlist 1,5,$(shell git --no-pager show --date=local --format="%ad" --name-only))
GIT_FLAGS  := -DCOMMIT='"$(GIT_COMMIT)"' -DDATE='"$(GIT_DATE)"'


RM := rm -f
