CC=gcc
CFLAGS=-Wall -g
fcompare: fcompare.o
fcompare_test: fcompare_test.o

clean:
	rm -f fcompare fcompare.o fcompare_test fcompare_test.o
