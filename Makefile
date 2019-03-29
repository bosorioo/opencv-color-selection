all:
  g++ -Wall src/main.cpp `pkg-config --cflags opencv` `pkg-config --libs opencv` -o project1
