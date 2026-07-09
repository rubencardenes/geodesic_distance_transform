# (c) Copyright Ruben Cardenes Almedia 1/10/2002

FLAGS=-O3 -I.
#FLAGS=-g -I.

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	LIBEXT=dylib
	SHARED_FLAG=-dynamiclib
else
	LIBEXT=so
	SHARED_FLAG=-shared
endif
LIB2D=libgeodesicDT2d.$(LIBEXT)

all: geodesicDT2d geodesicDT3d $(LIB2D)

geodesic_common.o: geodesic_common.c geodesic_common.h
	gcc ${FLAGS} -c geodesic_common.c -o geodesic_common.o

geodesicDT2d: geodesicDT2d.c geodesic_common.o
	gcc ${FLAGS} geodesicDT2d.c geodesic_common.o -lm -o geodesicDT2d

# Shared library used by the Python ctypes wrapper (geodesicDT2d.py).
$(LIB2D): geodesicDT2d.c geodesic_common.c
	gcc ${FLAGS} -fPIC ${SHARED_FLAG} geodesicDT2d.c geodesic_common.c -lm -o $(LIB2D)

geodesicDT3d: geodesicDT3d.cpp io_mhd.o geodesic_common.o
	g++ ${FLAGS} geodesicDT3d.cpp -lm io_mhd.o geodesic_common.o -o geodesicDT3d

io_mhd.o: io_mhd.cpp io_mhd.h geodesic_common.h
	c++ ${FLAGS} -c -o io_mhd.o io_mhd.cpp

test: tests/test_geodesic_common.c geodesic_common.c geodesic_common.h
	gcc ${FLAGS} tests/test_geodesic_common.c geodesic_common.c -lm -o tests/test_geodesic_common
	./tests/test_geodesic_common

clean:
	rm -f geodesicDT2d
	rm -f geodesicDT3d
	rm -f *.o
	rm -f *.so
	rm -f *.dylib
	rm -f tests/test_geodesic_common
	rm -f *~
