# Makefile.inc - common definitions used by all makefiles


src         := $(wildcard *.c)
obj         := $(src:.c=.o)
inc         := ../include
inc_flags   := -I$(inc)

LINKFLAGS   := 
LDFLAGS     :=  -lpthread
#LDFLAGS     := -fsanitize=address

CFLAGS   := -std=gnu11 -O2 -fPIC -Wall -g -Wpedantic
#CFLAGS   := -std=gnu11 -O2 -fPIC -Wall -g -Wpedantic -fsanitize=address
#DBGFLAGS := $(CFLAGS) -g -DDEBUG
#CFLAGS   += $(CFLAGS) 

GIT_COMMIT := $(shell git describe --abbrev=8 --dirty --always --tags)
GIT_DATE   := $(wordlist 1,5,$(shell git --no-pager show --date=local --format="%ad" --name-only))
GIT_FLAGS  := -DCOMMIT='"$(GIT_COMMIT)"' -DDATE='"$(GIT_DATE)"'


RM := rm -f
