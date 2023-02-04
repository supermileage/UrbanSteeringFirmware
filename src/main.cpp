#include <string>
#include <chrono>
#include <cmath>
#include <mbed.h>
#include <stdio.h>

#include "rtos.h"
#include "can_common.h"
#include "SPI_TFT_ILI9341.h"

#include "SharedProperty.h"
#include "SteeringDisplay.h"
#include "main.h"

using namespace std;
using namespace std::chrono;

#define DEBUG_THROTTLE 0 

#define DMS_DELTA 	0.003f
#define ACCESSORIES_TRANSMIT_INTERVAL 50
#define MOTOR_CONTROLLER_TRANSMIT_INTERVAL 50

#define DEBOUNCE_TIME 50
#define GESTURE_MARGIN 500
#define MIN_THROTTLE_OUTPUT 0.0f
#define MAX_THROTTLE_OUTPUT 255.0f
#define MIN_THROTTLE_INPUT 0.36f
#define MAX_THROTTLE_INPUT 0.455f
#define SLOPE (MAX_THROTTLE_OUTPUT - MIN_THROTTLE_OUTPUT) / (MAX_THROTTLE_INPUT - MIN_THROTTLE_INPUT)
#define OFFSET MAX_THROTTLE_OUTPUT - (SLOPE * MAX_THROTTLE_INPUT)

SPI_TFT_ILI9341 TFT(D11, D12, D13, D9, D0, A4);
DigitalOut sdCs(A0);
CAN can(D10, D2, 500000);
SteeringDisplay display(&TFT);
BufferedSerial pc(USBTX, USBRX); // Uncomment to turn on serial monitor

// Accessories
DigitalIn brake(D1,PullUp);

// Ready
AnalogIn dms(A1);
AnalogIn throttle(A6);
DigitalOut dmsLed(A5);

//shift registers
DigitalOut shiftClk(D3);
DigitalOut ledOut(D4);
DigitalOut shiftLatch(D5);
DigitalIn buttonIn(D6);

//CHANGES- shift reg
bool buttonState[8] = {};
bool ledState[8] = {};

//Joystick
AnalogIn joyX(A3);
AnalogIn joyY(A2);

Timer timerMotor;
Timer clockResetTimer;
Timer timerAccessories;

bool lastHazards = false;
Ticker timerFlash;

//Ticker ledTest;

// Properties to bind to steering GUI and CAN events
SharedProperty<data_t> dmsVal(0);
SharedProperty<data_t> ignitionVal(0);
SharedProperty<data_t> brakeVal(0);
SharedProperty<batt_t> batterySocVal(0);
SharedProperty<batt_t> batteryVoltageVal(0);
SharedProperty<speed_t> currentSpeedVal(0);
SharedProperty<throttle_t> throttleVal(0);
SharedProperty<data_t> lightsVal(0);
SharedProperty<data_t> turnLeftVal(0);
SharedProperty<data_t> turnRightVal(0);
SharedProperty<data_t> prevAccVal(0);
SharedProperty<data_t> lastGesture(0);
SharedProperty<steering_time_t> timeVal(steering_time_t { 0, 0 });

// global variables shared between main and display threads
int64_t g_lastTime = 0;

void initializeDisplay() {
	// initialize
	display.init();

	// add data display bindings
	display.addDynamicGraphicBinding(dmsVal, SteeringDisplay::Dms);
	display.addDynamicGraphicBinding(ignitionVal, SteeringDisplay::Ignition);
	display.addDynamicGraphicBinding(brakeVal, SteeringDisplay::Brake);
	display.addDynamicGraphicBinding(batterySocVal, SteeringDisplay::Soc);
	display.addDynamicGraphicBinding(batteryVoltageVal, SteeringDisplay::Voltage);
	display.addDynamicGraphicBinding(currentSpeedVal, SteeringDisplay::Speed);
	display.addDynamicGraphicBinding(throttleVal, SteeringDisplay::Power);
	display.addDynamicGraphicBinding(lightsVal, SteeringDisplay::Lights);
	display.addDynamicGraphicBinding(turnLeftVal, SteeringDisplay::LeftSignal);
	display.addDynamicGraphicBinding(turnRightVal, SteeringDisplay::RightSignal);
	display.addDynamicGraphicBinding(timeVal, SteeringDisplay::Minutes);
}

int main() {
	timerMotor.start();
	clockResetTimer.start();
	timerAccessories.start();
	//ledTest.attach(&setLed, 1.0);

	shiftClk.write(0);
	ledOut.write(0);
	shiftLatch.write(0);
	sdCs.write(1);

	// Initalize Accessories--it's impossible for left and right blinker to be on simultaneously so this will cause can update to be sent on boot
	prevAccVal.set(0xFF);

	initializeDisplay();

	Thread display_thread;
	display_thread.start(runSteeringDisplay);
	
	dmsLed.write(0);

	while (1) {
		handleTime();
		handle_accessories();
		handle_motor_inputs();
		receive_can();
		updateShiftRegs();
		setLedState();
	}
}

void handle_accessories() {
	if (duration_cast<milliseconds>(timerAccessories.elapsed_time()).count() > ACCESSORIES_TRANSMIT_INTERVAL) {
		char hazardsOn;
		char currentAcc = read_accessory_inputs(hazardsOn);
		if ((prevAccVal.value() != currentAcc)) {
			// turn hazards off
			if (hazardsOn) {
				const unsigned char data[] = { 0x2, 0x4 << 1, 0x5 << 1 };
				can.write(CANMessage(CAN_ACC_OPERATION, data, 3));
				wait_us(1000);
			}
			const char data[] = {0, currentAcc};
			can.write(CANMessage(CAN_ACC_OPERATION, data, 2));
			prevAccVal.set(currentAcc);
		}
		timerAccessories.reset();
	}
}

char read_accessory_inputs(char& hazards){
    //TODO
	lightsVal.set(0);
	char currentBrake = (char)(!brake.read());
	//TODO
    char horn = 0;
	//TODO
    hazards = 0;
	//TODO - change later for D3 and D4
    char turnLeft = 0;
    char turnRight = 0;
	//TODO
    char wiperVal = 0;

	// hazards on == both blinkers turned on at the same time
    if (hazards) {
		turnLeftVal.set(1);
        turnRightVal.set(1);
    } else {
		turnLeftVal.set(turnLeft);
		turnRightVal.set(turnRight);
	}

    char dataStr = (wiperVal << 6) | (turnLeftVal.value() << 5) | (turnRightVal.value() << 4) |
		(hazards << 3) |(horn << 2) | (currentBrake << 1) | lightsVal.value();
    return dataStr;
}

void handle_motor_inputs(){
	if (duration_cast<milliseconds>(timerMotor.elapsed_time()).count() > MOTOR_CONTROLLER_TRANSMIT_INTERVAL) {

		dmsVal.set(getDmsVal());
		//TODO - change for D6
		ignitionVal.set(0);
		brakeVal.set((char)!brake.read());

		throttle_t currentThrottle = get_throttle_val();
		
		if (!dmsVal.value() || !ignitionVal.value() || brakeVal.value()) {
			currentThrottle = 0;
		}

		throttleVal.set(currentThrottle);

		// Throttle Data
		const throttle_t throttleData[] = { throttleVal.value() };
		can.write(CANMessage(CAN_STEERING_THROTTLE, throttleData, 1));

		// Ready Data
		char ready = (brakeVal.value() << 2) | (dmsVal.value() << 1) | ignitionVal.value();
		const char readyData[] = { ready };
		can.write(CANMessage(CAN_STEERING_READY, readyData, 1));

        timerMotor.reset();
	}
}

throttle_t get_throttle_val() {
	float throttleAsFloat = throttle.read();

	if (throttleAsFloat <= MIN_THROTTLE_INPUT) {
		return (throttle_t)MIN_THROTTLE_OUTPUT;
	} else if (throttleAsFloat >= MAX_THROTTLE_INPUT) {
		return (throttle_t)MAX_THROTTLE_OUTPUT;
	}

	return (throttle_t)(throttleAsFloat * SLOPE + OFFSET);
}

data_t getDmsVal() {
	float dms_ctrl = dms.read();
	dmsLed.write(1);
	wait_us(40);
    float dms_val = dms.read();
	dmsLed.write(0);

    return (data_t)(dms_val > dms_ctrl + DMS_DELTA);
}

// Checks for gps speed / bms data updates from telemetry
void receive_can() {
	CANMessage msg;

	if (can.read(msg)) {
		if (msg.id == CAN_TELEMETRY_GPS_DATA) {
			currentSpeedVal.set((speed_t)msg.data[0]);
		} else if (msg.id == CAN_TELEMETRY_BMS_DATA) {
			batt_t soc, voltage;
			memcpy((void*)&soc, (void*)msg.data, 4);
			memcpy((void*)&voltage, (void*)(msg.data + 4), 4);
			batterySocVal.set(soc);
			batteryVoltageVal.set(voltage);
		}
	}
}

void handleTime() {

	if(!buttonState[0]) {
		clockResetTimer.reset();
	} 

	int64_t currentTime =clockResetTimer.read_ms();

	timeVal.set(steering_time_t { (int)currentTime / 1000 / 60, (int)currentTime / 1000 % 60 });
}

void runSteeringDisplay() {
	while (1) {
		display.run();
	}
}

void updateShiftRegs() {

	shiftLatch.write(1);
    for(int i = 7; i >= 0; i--){
		wait_us(500); // TODO: Debug hardware issue
        buttonState[i] = buttonIn.read();
        shiftClk.write(1);
        shiftClk.write(0);
    }
	// printf("%1d%1d%1d%1d%1d%1d%1d%1d\n", buttonState[0], buttonState[1], buttonState[2], buttonState[3], buttonState[4], buttonState[5], buttonState[6], buttonState[7]);
    shiftLatch.write(0);
	for(int i = 8; i >= 0; i--){
        ledOut.write(!ledState[i]);
        shiftClk.write(1);
        shiftClk.write(0);
    }

}

void blinkHazardLed() {
	ledState[6] = !ledState[6];
}

void setLedState() {
	// Set wiper LED
	ledState[1] = buttonState[3];
	// Set lights LED
	ledState[2] = buttonState[4];
	// Set Horn LED
	ledState[3] = buttonState[5];
	// Set Ignition LED
	ledState[4] = !buttonState[6];
	ledState[5] = buttonState[6];
	if(buttonState[7] != lastHazards) {
		if(buttonState[7]) {
			timerFlash.attach(blinkHazardLed, 500ms);
			ledState[6] = true;
		} else {
			timerFlash.detach();
			ledState[6] = false;
		}
		lastHazards = buttonState[7];
	}

}


void ledOn(int ledPositions[], int numLedPositions){
    int leds[8] = {0}; // All LEDs ON - using ! to counteract this
    for(int i = 0; i < numLedPositions; i++){
        leds[ledPositions[i]] = 1; 
    }
    shiftLatch.write(0);
    for(int i = 0; i < 8; i++){
        ledOut.write(!leds[i]); //Using ! to counter all LEDs initialised as ON
        shiftClk.write(1);
        wait_us(1000);
        shiftClk.write(0);
    }
    shiftLatch.write(1);
}

void print_buttons_bitwise(uint8_t buttons) {
    for (int i = 7; i >= 0; i--) {
        printf("%d", (buttons >> i) & 1);
    }
    printf("\n");
}



/*	for(int i = 0; i < 8; i++) {
    if((buttons >> i) & 1) {
        ledOn(i);  // function to turn on the LED at position i
    }
}
*/

///////////////  Testing random functions
/*
void buttonState(){
	shiftClk = 0;

	buttons = (buttons << 1) | buttonIn.read();

	leds = (buttons & 0x01) ? (leds | 0x01) : (leds & 0xFF);

	ledOut = leds;

	shiftClk = 1;
	wait_us(10);
	shiftClk = 0;

	shiftLatch = 1;
	wait_us(10);
	shiftLatch = 0;

	printf("Button state: %d\n", buttons);
}


void buttonTest(int buttonPress){
	shiftClk = 1;
	shiftLatch = 0;
	if(buttonPress == 1) ledOut.write(1);
}

void buttonRead(int buttonPress){
	int ledPos;
	shiftLatch.write(0);
	for(int i = 0; i <8; i++){
		if(i == 3 && buttonPress == 1){
			ledPos = 1;
			ledOn(ledPos);
		}

		else if(i == 4 && buttonPress == 1){
			ledPos = 2;
			ledOn(ledPos);
		}

		else if(i == 5 && buttonPress == 1){
			ledPos = 3;
			ledOn(ledPos);
		}

		else if(i == 6 && buttonPress == 0){
			ledPos = 4;
			ledOn(ledPos);
		}

		else if(i == 6 && buttonPress == 1){
			ledPos = 5;
			ledOn(ledPos);
		}

		else if(i == 7 && buttonPress == 1){
			ledPos = 6;
			ledOn(ledPos);
		}
		shiftClk.write(1);
		shiftClk.write(0);
	}
	shiftLatch.write(1);
}

void setLeds() {
	// Set all LEDs
	shiftLatch.write(0);
	for(int i = 0; i < 8; i++){
		ledOut.write(0);
		shiftClk.write(1);
		shiftClk.write(0);
	}
	shiftLatch.write(1);
}




/////////////////////////////////////////////////////////////////////

void updateShiftRegs() {
	// Update button status and output LED status at the same time
	// Return state of all buttons
	shiftLatch.write(1);
	for(int i =0; i < 8; i++){
		buttons = (buttons << 1) | buttonIn.read();
		shiftClk.write(1);
		wait_us(1000);
		shiftClk.write(0);
		wait_us(1000);
	}
	shiftLatch.write(0);
	print_buttons_bitwise(buttons);

	for(int i = 0; i < 6; i++) {
		if((buttons >> 3) & 1) {
			ledOn(7);
		}
		if((buttons >> 4) & 1) {
			ledOn(6);
		}
		if((buttons >> 5) & 1) {
			ledOn(5);
		}
		if((buttons >> 6) & 0) {
			ledOn(4);
		}		
		if((buttons >> 6) & 1) {
			ledOn(3);
		}
		if((buttons >> 7) & 1) {
			ledOn(2);
		}
	}
}





void ledOn(int ledPos){
//	shiftLatch.write(1);
	shiftLatch.write(0);
	for(int i = 0; i < 8; i++){
		if(ledPos == i){
			ledOut.write(0);
		} else {
			ledOut.write(1);
		}
		shiftClk.write(1);
		wait_us(1000);
		shiftClk.write(0);
	}
	shiftLatch.write(1);
}

*/
