all:
	gcc x11.c -o x11 -lX11 
i2c:
	gcc i2c.c -o i2c
clean:
	rm x11 i2c
