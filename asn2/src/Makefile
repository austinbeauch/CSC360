.phony all:
all: p2 queue.c queue.h

p2: p2.c queue.c
	gcc queue.c -pthread p2.c -o mts


.PHONY clean:
clean:
	-rm -rf *.o *.exe
