#include <unistd.h>				//Needed for I2C port
#include <fcntl.h>				//Needed for I2C port
#include <sys/ioctl.h>			//Needed for I2C port
#include <linux/i2c-dev.h>		//Needed for I2C port
#include <stdio.h>
#include <wiringPiI2C.h>


#define REG_STATUS 0x00
#define REG_WRITE 0x01
#define REG_READ 0x02
#define V_HIGH 0x08
#define V_LOW 0x09
#define B_HIGH 0x0A
#define B_LOW 0x0B
#define G_HIGH 0x0C
#define G_LOW 0x0D
#define Y_HIGH 0x0E
#define Y_LOW 0x0F
#define O_HIGH 0x10
#define O_LOW 0x11
#define R_HIGH 0x12
#define R_LOW 0x13
#define CONTROL 0x04
#define INT_T 0x05

#define STATUS_RX_VALID 0x01
#define STATUS_TX_VALID 0x02
#define DEVICE_ADDRESS 0x49

int file_i2c;
int length;
unsigned char buffer[60] = {0};

int read_reg(int fd, unsigned char VREG) {
    int status = 0;
    do {
    	status = wiringPiI2CReadReg8(fd, REG_STATUS);
    } while(status & STATUS_TX_VALID);
    
    wiringPiI2CWriteReg8(fd, REG_WRITE, VREG);
    
    do {
        status = wiringPiI2CReadReg8(fd, REG_STATUS);
    } while(!(status & STATUS_RX_VALID));

    return wiringPiI2CReadReg8(fd, REG_READ);
}

int write_reg(int fd, unsigned char VREG, unsigned char val) {
    int status = 0;
    do {
    	status = wiringPiI2CReadReg8(fd, REG_STATUS);
    } while(status & STATUS_TX_VALID);
    
    wiringPiI2CWriteReg8(fd, REG_WRITE, VREG | 0x80);
    
    do {
        status = wiringPiI2CReadReg8(fd, REG_STATUS);
    } while(status & STATUS_TX_VALID);

    wiringPiI2CWriteReg8(fd, REG_WRITE, val);
    return 1;
}


int main() {
    int fd = wiringPiI2CSetup(DEVICE_ADDRESS);
    unsigned short vals[6] = {0};
    int high, low;
    if (fd == -1) {
	printf("asdf");
	return -1;
    }
    /* TODO: Reset by pulling reset pun low */
    int stat = wiringPiI2CReadReg8(fd, REG_STATUS);
    //write_reg(fd, CONTROL, 0x88);
    write_reg(fd, CONTROL, 0x38);
    //int initial_read = wiringPiI2CReadReg8(fd, REG_READ);
    //printf("%d\n", initial_read);

    write_reg(fd, INT_T, 0x4F);
    while (1) {
    high = read_reg(fd, V_HIGH);
    low  = read_reg(fd, V_LOW);
    vals[0] = (unsigned short) (high << 8 | low);

    high = read_reg(fd, B_HIGH);
    low  = read_reg(fd, B_LOW);
    vals[1] = (unsigned short) (high << 8 | low);

    high = read_reg(fd, G_HIGH);
    low  = read_reg(fd, G_LOW);
    vals[2] = (unsigned short) (high << 8 | low);

    high = read_reg(fd, Y_HIGH);
    low  = read_reg(fd, Y_LOW);
    vals[3] = (unsigned short) (high << 8 | low);

    high = read_reg(fd, O_HIGH);
    low  = read_reg(fd, O_LOW);
    vals[4] = (unsigned short) (high << 8 | low);

    high = read_reg(fd, R_HIGH);
    low  = read_reg(fd, R_LOW);
    vals[5] = (unsigned short) (high << 8 | low);
    
    unsigned char control = read_reg(fd, CONTROL);
    unsigned char int_t = read_reg(fd, INT_T);
    printf("control: %02x\n", control);
    printf("int time: %02x\n", int_t);
    for (int i = 0; i < 6; i++) {
        printf("%d\n", vals[i]);
    }
    }
    return 0;
}
