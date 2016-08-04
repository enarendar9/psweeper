CC := gcc
SRCS := connector.c collector.c sweeper.c
LIBDIR := lib
LIBS := freeblist.o dispatcher.o
STDLIBS := pthread

OBJS := $(patsubst %.c,%.o,$(SRCS))
LIBOBJS := $(patsubst %.o,$(LIBDIR)/%.o,$(LIBS))

%.o: %.c
	gcc -c -g -o $@ $<

sweeper: $(OBJS) $(LIBOBJS)
	gcc -o $@ -g $(OBJS) $(LIBOBJS) -l $(STDLIBS)

clean:
	-rm -f $(OBJS)
	make -C $(LIBDIR) clean

all: sweeper

.PHONY: all clean
