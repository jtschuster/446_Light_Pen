CFLAGS= -lwiringPi -lX11 -lXtst -pthread -Wall -Wextra #-Wpedantic
DEPS= src/fbuff.c src/cursor.c

all:
	gcc main.c $(DEPS) -o light_pen $(CFLAGS)
opt:
	gcc main.c $(DEPS) -o light_pen $(CFLAGS) -O3
debug:
	gcc main.c $(DEPS) -o light_pen $(CFLAGS) -g 
single:
	gcc main3.c -o light_pen $(CFLAGS) -O3
test:
	gcc test.c $(DEPS) -o test $(CFLAGS) -O3
