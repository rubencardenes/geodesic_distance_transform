# (c) Copyright Ruben Cardenes Almedia 1/10/2002

FLAGS=-O3 -I.
#FLAGS=-g -I.

all: geodesicDT2d geodesicDT3d io_mhd.o

geodesicDT2d: geodesicDT2d.c 
	gcc ${FLAGS} geodesicDT2d.c -lm -o geodesicDT2d

geodesicDT3d: geodesicDT3d.cpp io_mhd.o
	g++ ${FLAGS} geodesicDT3d.cpp -lm io_mhd.o -o geodesicDT3d

clean:
	rm -f geodesicDT2d
	rm -f geodesicDT3d
	rm -f *.o
	rm -f *~
