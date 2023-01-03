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
// BufferedSerial pc(USBTX, USBRX); // tx, rx

// Accessories
DigitalIn brake(D1,PullUp);

// Ready
AnalogIn dms(A1);
AnalogIn throttle(A6);
DigitalOut dmsLed(A5);

//shift registers
DigitalOut shift_clk(D3);
DigitalOut led_out(D4);
DigitalOut shiftLatch(D5);
DigitalIn buttonIn(D6);

//Joystick
AnalogIn joyX(A3);
AnalogIn joyY(A2);

Timer timerMotor;
Timer clockResetTimer;
Timer timerAccessories;
Timer timerFlash;

// Properties to bind to steering GUI and CAN events
SharedProperty<data_t> dmsVal(0);
SharedProperty<data_t> ignitionVal(0);
SharedProperty<data_t> brakeVal(0);
SharedProperty<batt_t> batterySoc(0);
SharedProperty<batt_t> batteryVoltage(0);
SharedProperty<speed_t> currentSpeed(0);
SharedProperty<throttle_t> throttleVal(0);
SharedProperty<data_t> lightsVal(0);
SharedProperty<data_t> turnLeftVal(0);
SharedProperty<data_t> turnRightVal(0);
SharedProperty<data_t> prevAccVal(0);
SharedProperty<data_t> lastGesture(0);
SharedProperty<steering_time_t> timeVal(steering_time_t { 0, 0 });

// global variables shared between main and display threads
int64_t g_lastGestureTime = 0;
int64_t g_lastTime = 0;
char g_lastGesture = 0;

void initializeDisplay() {
	// initialize
	display.init();

	// add data display bindings
	display.addDynamicGraphicBinding(dmsVal, SteeringDisplay::Dms);
	display.addDynamicGraphicBinding(ignitionVal, SteeringDisplay::Ignition);
	display.addDynamicGraphicBinding(brakeVal, SteeringDisplay::Brake);
	display.addDynamicGraphicBinding(batterySoc, SteeringDisplay::Soc);
	display.addDynamicGraphicBinding(batteryVoltage, SteeringDisplay::Voltage);
	display.addDynamicGraphicBinding(currentSpeed, SteeringDisplay::Speed);
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

	shift_clk.write(0);
	led_out.write(0);
	shiftLatch.write(0);
	sdCs.write(1);

	// Initalize Accessories--it's impossible for left and right blinker to be on simultaneously so this will cause can update to be sent on boot
	prevAccVal = 0xFF;

	initializeDisplay();

	Thread display_thread;
	display_thread.start(runSteeringDisplay);
	
	dmsLed.write(0);

	while (1) {
		handleTime();
		handle_accessories();
		handle_motor_inputs();
		receive_can();
	}
}

void handle_accessories() {
	if (duration_cast<milliseconds>(timerAccessories.elapsed_time()).count() > ACCESSORIES_TRANSMIT_INTERVAL) {
		char hazardsOn;
		char currentAccVal = read_accessory_inputs(hazardsOn);
		if ((prevAccVal.getValue() != currentAccVal)) {
			// turn hazards off
			if (hazardsOn) {
				const unsigned char data[] = { 0x2, 0x4 << 1, 0x5 << 1 };
				can.write(CANMessage(CAN_ACC_OPERATION, data, 3));
				wait_us(1000);
			}
			const char data[] = {0, currentAccVal};
			can.write(CANMessage(CAN_ACC_OPERATION, data, 2));
			prevAccVal = currentAccVal;
		}
		timerAccessories.reset();
	}
}

char read_accessory_inputs(char& hazardsVal){
    //TODO
	lightsVal = 0;
	char brakeVal = (char)(!brake.read());
	//TODO
    char hornVal = 0;
	//TODO
    hazardsVal = 0;
	//TODO - change later for D3 and D4
    char turnLeft = 0;
    char turnRight = 0;
	//TODO
    char wiperVal = 0;

	// hazards on == both blinkers turned on at the same time
    if (hazardsVal) {
		turnLeftVal = 1;
        turnRightVal = 1;
    } else {
		turnLeftVal = turnLeft;
		turnRightVal = turnRight;
	}

    char dataStr = (wiperVal << 6) | (turnLeftVal << 5) | (turnRightVal << 4) | (hazardsVal << 3) | (hornVal << 2) | (brakeVal << 1) | lightsVal.getValue();
    return dataStr;
}

void handle_motor_inputs(){
	if (duration_cast<milliseconds>(timerMotor.elapsed_time()).count() > MOTOR_CONTROLLER_TRANSMIT_INTERVAL) {

		dmsVal = getDmsVal();
		//TODO - change for D6
		ignitionVal = 0;
		brakeVal = (char)!brake.read();

		throttle_t currentThrottle = get_throttle_val();
		
		if (!dmsVal.getValue() || !ignitionVal.getValue() || brakeVal.getValue()) {
			currentThrottle = 0;
		}

		throttleVal = currentThrottle;

		// Throttle Data
		const throttle_t throttleData[] = { throttleVal.getValue() };
		can.write(CANMessage(CAN_STEERING_THROTTLE, throttleData, 1));

		// Ready Data
		char readyVal = (brakeVal << 2) | (dmsVal << 1) | ignitionVal.getValue();
		const char readyData[] = { readyVal };
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
			currentSpeed = (unsigned)msg.data[0];
		} else if (msg.id == CAN_TELEMETRY_BMS_DATA) {
			float soc, voltage;
			memcpy((void*)&soc, (void*)msg.data, 4);
			memcpy((void*)&voltage, (void*)(msg.data + 4), 4);
			batterySoc = soc;
			batteryVoltage = voltage;
		}
	}
}

void handleTime() {
	//TODO - change for D6
	char thisGesture = 0;
	int64_t currentTime = duration_cast<milliseconds>(clockResetTimer.elapsed_time()).count();

	// update timeVal
	if (g_lastGestureTime + DEBOUNCE_TIME < currentTime && g_lastGesture != thisGesture) {
		if (currentTime <= g_lastGestureTime + GESTURE_MARGIN) {
			currentTime = 0;
			g_lastTime = -1;
			g_lastGestureTime = 0;
			clockResetTimer.reset();
		} else {
			g_lastGestureTime = currentTime;
		}
	}
	
	timeVal = steering_time_t { (int)currentTime / 1000 / 60, (int)currentTime / 1000 % 60 };
	g_lastGesture = thisGesture;
}

void runSteeringDisplay() {
	while (1) {
		display.run();
	}
}