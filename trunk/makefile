CPP = g++

PKGCONFIG = `pkg-config --cflags --libs playerc++`
#LIBS = -I /usr/include/estools -I /usr/include/festival/ -lestools -lestbase -leststring -lesd -lncurses -ltermcap -lstdc++

all: hallwaydrive logdpslam log2jpeg rfid pgm

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
	
#Other utility files
player2dpslam: player2dpslam.cpp
	$(CPP) -o player2dpslam player2dpslam.cpp
	
logdpslam: logdpslam.cc
	$(CPP) -o logdpslam logdpslam.cc
	
log2jpeg: logs/log2jpeg.cc
	$(CPP) -o logs/log2jpeg logs/log2jpeg.cc

rfid: rfid.cpp rfid.h comparators.h
	$(CPP) -c rfid.cpp
	
pgm: pgm.cpp pgm.h
	$(CPP) -c pgm.cpp
