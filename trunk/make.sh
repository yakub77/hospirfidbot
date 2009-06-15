g++ -o hallwaydrive -I /usr/include/festival/ -lX11 -lm $(pkg-config --cflags --libs playerc++) hallwaydrive.cc
