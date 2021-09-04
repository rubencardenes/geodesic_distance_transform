# (c) Copyright Ruben Cardenes Almedia 1/10/2002

all: geodesicDT2d geodesicDT3d

geodesicDT2d: geodesicDT2d.c 
	gcc -g -O2 geodesicDT2d.c -lm -o geodesicDT2d

geodesicDT3d: geodesicDT3d.c
	gcc -g -O2 geodesicDT3d.c -lm -o geodesicDT3d

clean:
	rm -f geodesicDT2d
	rm -f geodesicDT3d
	rm -f *~
