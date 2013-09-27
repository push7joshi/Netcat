#use gcc compiler
CC=gcc

#use flags -lrt for time keeping
CFLAGS=-lcrypto -lssl

all: ncat ncatCrypto

ncat:
	$(CC) netcat_part.c -o ncat

ncatCrypto:
	$(CC) $(CFLAGS) netcat_hmac.c -o ncatcrypto

clean:
	rm ncat Ncatcrypto
