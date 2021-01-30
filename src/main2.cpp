#include "mbed.h"
// New message from Spencer 
// conf
DigitalOut led(LED1);
// This is a message from Spencer :)

int main() {
	// Demo code
	// Jeffs message
	while(true) {
		led = !led;
		wait(22);
	}	
}


// Another message
