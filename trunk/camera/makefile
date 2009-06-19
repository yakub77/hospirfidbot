CPP = gcc

PKGCONFIG = `pkg-config --cflags --libs playerc++`
#LIBS = -I /usr/include/estools -I /usr/include/festival/ -lestools -lestbase -leststring -lesd -lncurses -ltermcap -lstdc++

all: cameratest

clobber: clean
	rm -f *~ \#*\# core

clean:
	rm -f main *.o

cameratest: camera.cc
	$(CPP) $(LIBS) -o cameratest camera.cc $(PKGCONFIG)
