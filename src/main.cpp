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

#define DMS_THRESH 	0.6f
#define DMS_DELTA 	0.1f
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

SPI_TFT_ILI9341 TFT(D11, D12, D13, A7, D0, A3);
CAN can(D10, D2, 500000);
SteeringDisplay display(&TFT);
// BufferedSerial pc(USBTX, USBRX); // tx, rx

// Accessories
DigitalIn lights(A0,PullDown);
DigitalIn brake(D1,PullUp);
DigitalIn horn(D5,PullDown);
DigitalIn hazards(D9,PullDown);
DigitalIn rightBlinker(D4,PullDown);
DigitalIn leftBlinker(D3,PullDown);
DigitalIn wipers(A2,PullDown);

// Ready
DigitalIn ignition(D6,PullDown);
AnalogIn dms(A1);
AnalogIn throttle(A6);
DigitalOut dmsLed(A5);

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

	// Initalize Accessories--it's impossible for left and right blinker to be on simultaneously so this will cause can update to be sent on boot
	prevAccVal = 0xFF;

	initializeDisplay();

	Thread display_thread;
	display_thread.start(runSteeringDisplay);
	
	dmsLed.write(1);

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
    lightsVal = (char)(lights.read());
	char brakeVal = (char)(!brake.read());
    char hornVal = (char)(horn.read());
    hazardsVal = (char)(hazards.read());
    char turnLeft = (char)(leftBlinker.read());
    char turnRight = (char)(rightBlinker.read());
    char wiperVal = (char)(wipers.read());

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
		ignitionVal = (char)ignition.read();
		brakeVal = (char)!brake.read();

		uint8_t currentThrottle = get_throttle_val();
		
		if (!dmsVal.getValue() || !ignitionVal.getValue() || brakeVal.getValue()) {
			currentThrottle = 0;
		}

		throttleVal = currentThrottle;

		// Throttle Data
		const unsigned char throttleData[] = { throttleVal.getValue() };
		can.write(CANMessage(CAN_STEERING_THROTTLE, throttleData, 1));

		// Ready Data
		char readyVal = (brakeVal << 2) | (dmsVal << 1) | ignitionVal.getValue();
		const char readyData[] = { readyVal };
		can.write(CANMessage(CAN_STEERING_READY, readyData, 1));

        timerMotor.reset();
	}
}

unsigned char get_throttle_val() {
	float throttleAsFloat = throttle.read();

	if (throttleAsFloat <= MIN_THROTTLE_INPUT) {
		return (unsigned char)MIN_THROTTLE_OUTPUT;
	} else if (throttleAsFloat >= MAX_THROTTLE_INPUT) {
		return (unsigned char)MAX_THROTTLE_OUTPUT;
	}

	return (unsigned char)(throttleAsFloat * SLOPE + OFFSET);
}

data_t getDmsVal() {
	float dms_ctrl = dms.read();
	dmsLed.write(0);
	wait_us(40);
    float dms_val = dms.read();
	dmsLed.write(1);

    return (data_t)(dms_val <= DMS_THRESH && (dms_val < dms_ctrl - DMS_DELTA));
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
	char thisGesture = ignition.read();
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