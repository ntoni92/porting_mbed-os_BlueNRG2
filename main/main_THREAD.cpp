#include "mbed.h"

DigitalOut led1(LED1);
DigitalOut led2(LED2);
InterruptIn button(PUSH1);
Serial pc(USBTX,USBRX);
//Thread thread;

void flip(){
	led2 = !led2;
}

void on(){
	led2 = 1;
}

void off(){
	led2 = 0;
}

void led2_thread() {
    while (true) {
        led2 = 1;
        pc.printf("thread2\r\n");
        //wait(1);
    }
}

// main() runs in its own thread in the OS
int main() {
	//thread.start(led2_thread);
	on();
	button.rise(&flip);
	button.fall(&flip);
    while (true) {
        led1 = !led1;
        wait(1);
        pc.printf("thread main\r\n");
        //wait(5);
        //pc.printf("CIAO\r\n");
        //while(1){;}
     }
}
