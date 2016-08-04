
freeblist.o: freeblist.c
	gcc -c -g -o $@ $?

dispatcher.o: dispatcher.c
	gcc -c -g -o $@ $?

.PHONY: all clean

all: freeblist.o dispatcher.o

clean:
	-rm -f *.o
