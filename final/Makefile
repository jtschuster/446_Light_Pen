CFLAGS= -lwiringPi -lX11 -lXtst -pthread -Wall
DEPS= src/fbuff.c

all:
	gcc main.c $(DEPS) -o light_pen $(CFLAGS)
opt:
	gcc main.c $(DEPS) -o light_pen $(CFLAGS) -O3
debug:
	gcc main.c $(DEPS) -o light_pen $(CFLAGS) -g 
single:
	gcc main3.c -o light_pen $(CFLAGS) -O3