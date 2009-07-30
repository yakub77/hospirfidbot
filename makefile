CPP = g++ -g
JAVA = javac

PKGCONFIG = `pkg-config --cflags --libs playerc++`
#LIBS = -I /usr/include/estools -I /usr/include/festival/ -lestools -lestbase -leststring -lesd -lncurses -ltermcap -lstdc++

#all: changeodometry player2dpslam

all: makeheatmaps hallwaydrive player2dpslam changeodometry SimulateRFID QuickRFIDView AutoQuantize RFIDdriver RFIDdriver.so ViewHeatmaps ViewVideo log2jpeg

clobber: clean
	rm -f *~ \#*\# core

clean:
	rm -f main *.o *.so

RFIDdriver.so: RFIDdriver.o
	$(CPP)  $(LIBS) -shared -nostartfiles -o $@ $^ $(PKGCONFIG)

RFIDdriver: RFIDdriver.h RFIDdriver.cpp
	$(CPP) $(LIBS) -c RFIDdriver.cpp $(PKGCONFIG)

AutoQuantize: AutoQuantize.cpp pgm.o
	$(CPP) -o AutoQuantize AutoQuantize.cpp pgm.o

QuickRFIDView: QuickRFIDView.cpp rfid.o
	$(CPP) -o QuickRFIDView QuickRFIDView.cpp rfid.o

SimulateRFID: SimulateRFID.cpp pgm.o
	$(CPP) -o SimulateRFID SimulateRFID.cpp pgm.o

ViewVideo: ViewVideo.java PGMImage.java
	$(JAVA) ViewVideo.java PGMImage.java

ViewHeatmaps: ViewHeatmaps.java PGMImage.java
	$(JAVA) ViewHeatmaps.java PGMImage.java

makeheatmaps: makeheatmaps.o rfid.o pgm.o heatmap.o
	$(CPP) $(LIBS) -o makeheatmaps makeheatmaps.o rfid.o pgm.o heatmap.o
	
makeheatmaps.o: makeheatmaps.cpp
	$(CPP) $(LIBS) -c makeheatmaps.cpp

hallwaydrive: hallwaydrive.o speak.o
	$(CPP) $(LIBS) -o hallwaydrive hallwaydrive.o speak.o $(PKGCONFIG)

hallwaydrive.o: hallwaydrive.cc
	$(CPP) $(LIBS) -c hallwaydrive.cc $(PKGCONFIG)

speak.o: speak.h speak.cpp
	$(CPP) $(LIBS) -c speak.cpp

rfid.o: rfid.cpp rfid.h comparators.h
	$(CPP) -c rfid.cpp
	
pgm.o: pgm.cpp pgm.h
	$(CPP) -c pgm.cpp
	
heatmap.o: heatmap.cpp heatmap.h
	$(CPP) -c heatmap.cpp

#Other utility files
player2dpslam: player2dpslam.cpp
	$(CPP) -o player2dpslam player2dpslam.cpp
	
player2dold: changeodometry.cpp
	$(CPP) -o changeodometry changeodometry.cpp
	
log2jpeg: logs/log2jpeg.cc
	$(CPP) -o logs/log2jpeg logs/log2jpeg.cc



