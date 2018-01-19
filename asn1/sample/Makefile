.phony all:
all: ssi

ssi: ssi.c
	gcc ssi.c -lreadline -lhistory -ltermcap -o ssi


.PHONY clean:
clean:
	-rm -rf *.o *.exe
