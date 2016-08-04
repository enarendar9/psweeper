CC := gcc
TBSRCS := test_blist.c
LIBDIR := ../lib
TBLIBS := freeblist.o

TBOBJS := $(patsubst %.c,%.o,$(TBSRCS))
TBLIBOBJS := $(patsubst %.o,$(LIBDIR)/%.o,$(TBLIBS))

%.o: %.c
	gcc -c -g -o $@ $<

tblist: $(TBOBJS) $(TBLIBOBJS)
	gcc -o $@ -g $^

clean:
	-rm -f $(TBOBJS)
