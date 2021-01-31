#include "mbed.h"

DigitalIn led(LED1);

int main() {
	while(true) {
		led = !led;
		wait(5);	
	}	
}


