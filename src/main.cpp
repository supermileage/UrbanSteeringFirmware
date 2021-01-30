#include "mbed.h"

DigitalOut led(LED1);
// This is a message from Spencer :)

int main() {
	// Demo code
	while(true) {
		led = !led;
		wait(55);	
	}	
}
