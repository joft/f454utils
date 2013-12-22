SRCS = startssh.c

EXE = startssh

CFLAGS ?= -Wall -Werror -O2 -g
LDFLAGS ?=


OBJS = $(foreach src,$(SRCS),$(subst .c,.o,$(src)))

STRIP ?= strip
ifneq ($(CROSS_COMPILE),)
CC = $(CROSS_COMPILE)cc
CCOUT := $(shell $(CC) --version 2>/dev/null)
ifndef CCOUT
CC = $(CROSS_COMPILE)gcc
endif
STRIP = $(CROSS_COMPILE)strip
endif

all: $(EXE) $(EXE)_strip

$(EXE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(EXE)_strip: $(EXE)
	$(STRIP) -g -v -o $@ $^

%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -MM $(CFLAGS) $< -o $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

-include $(SRCS:.c=.d)

%.o: %.c %.d
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f *~
	rm -f *.o *.d $(EXE) $(EXE)_strip
