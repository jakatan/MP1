all: crc.o crsd.o
	
clean:
	rm client server

crc.o: crc.c interface.h
	gcc  -pthread -o client crc.c 

crsd.o: crsd.c interface.h
	gcc  -pthread -o server crsd.c 
	