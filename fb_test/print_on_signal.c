#include <stdio.h>
#include <wiringPi.h>

int num=0;
void signal_callback() {
    num++;
    printf("got a signal from pin 29 %d\n", num);
}

int main() {
    if (wiringPiSetup() == -1)
        return -1;
    pinMode(29, INPUT);
    wiringPiISR(29, INT_EDGE_RISING, (void*)&signal_callback);
    while (1) {
//        printf("%d", digitalRead(29));
    }
}
