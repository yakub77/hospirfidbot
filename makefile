CPP = gcc

PKGCONFIG = `pkg-config --cflags --libs playerc++`
#LIBS = -I /usr/include/estools -I /usr/include/festival/ -lestools -lestbase -leststring -lesd -lncurses -ltermcap -lstdc++

all: hallwaydrive

clobber: clean
	rm -f *~ \#*\# core

clean:
	rm -f main *.o

hallwaydrive: hallwaydrive.o speak.o
	$(CPP) $(LIBS) -o hallwaydrive hallwaydrive.o speak.o $(PKGCONFIG)

hallwaydrive.o: hallwaydrive.cc
	$(CPP) $(LIBS) -c hallwaydrive.cc $(PKGCONFIG)

speak.o: speak.h speak.cpp
	$(CPP) $(LIBS) -c speak.cpp
