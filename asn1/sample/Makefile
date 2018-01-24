.phony all:
all: ssi

ssi: ssi.c linkedlist.c
	gcc ssi.c linkedlist.c -lreadline -lhistory -ltermcap -o ssi


.PHONY clean:
clean:
	-rm -rf *.o *.exe
