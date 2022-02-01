all: crc.o crsd.o
	
clean:
	rm client server

crc.o: crc.c interface.h
	gcc crc.c -o client

crsd.o: crsd.c interface.h
	gcc crsd.c -o server
	