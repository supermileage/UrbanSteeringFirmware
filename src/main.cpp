#include "mbed.h"

DigitalOut led(LED1);

int main() {
	// Demo code
	while(true) {
		led = !led;
		wait(5);	
	}	
}
