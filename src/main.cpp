#include <string>
#include <cmath>

#include "stdio.h"
#include "rtos.h"

#include "can_common.h"
#include "SPI_TFT_ILI9341.h"

#include "Arial12x12.h"
#include "Arial28x28.h"
#include "font_big.h"
#include "graphics.h"

#include "SharedProperty.h"
#include "SteeringDisplay.h"

#include "main.h"

using namespace std;

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

SPI_TFT_ILI9341 TFT(D11, D12, D13, A7, D0, A3, "TFT");
// BufferedSerial pc(USBTX, USBRX); // tx, rx
CAN can(D10, D2, 500000);

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
Timer timerDisplay;
Timer timerAccessories;
Timer timerFlash;

// global variables shared between main and display threads
int g_lastGestureTime = 0;
int g_lastTime = 0;
char g_lastGesture = 0;

// Properties to bind to steering GUI and CAN events
SharedProperty<data_t> dmsVal(0);
SharedProperty<data_t> ignitionVal(0);
SharedProperty<data_t> brakeVal(0);
SharedProperty<batt_t> batterySoc(0);
SharedProperty<batt_t> batteryVoltage(0);
SharedProperty<speed_t> currentSpeed(0);
SharedProperty<throttle_t> currentThrottle(0);
SharedProperty<data_t> lightsVal(0);
SharedProperty<data_t> turnLeft(0);
SharedProperty<data_t> turnRight(0);
SharedProperty<data_t> prevAccVal(0);
SharedProperty<data_t> lastGesture(0);

void initializeDisplay() {
	display.init();
	display.addDynamicGraphicBinding(dmsVal, SteeringDisplay::Dms);
	display.addDynamicGraphicBinding(ignitionVal, SteeringDisplay::Ignition);
	display.addDynamicGraphicBinding(brakeVal, SteeringDisplay::Brake);
	display.addDynamicGraphicBinding(batterySoc, SteeringDisplay::Soc);
	display.addDynamicGraphicBinding(batteryVoltage, SteeringDisplay::Voltage);
	display.addDynamicGraphicBinding(currentSpeed, SteeringDisplay::Speed);
	display.addDynamicGraphicBinding(currentThrottle, SteeringDisplay::Power);
	display.addDynamicGraphicBinding(lightsVal, SteeringDisplay::Lights);
	display.addDynamicGraphicBinding(turnLeft, SteeringDisplay::LeftSignal);
	display.addDynamicGraphicBinding(turnRight, SteeringDisplay::RightSignal);
}

SteeringDisplay display(&TFT);

int main() {
	timerMotor.start();
	timerDisplay.start();
	timerAccessories.start();

	// Initalize Accessories--it's impossible for left and right blinker to be on simultaneously so this will cause can update to be sent on boot
	prevAccVal = 0xFF;

	initializeDisplay();
	Thread display_thread;
	display_thread.start(display_task);

	dmsLed.write(1);

	while (1) {
		handle_reset_gesture();

		handle_accessories();

		handle_motor_inputs();
		
		receive_can();
	}
}

void handle_accessories() {
	if (timerAccessories.read_ms() > ACCESSORIES_TRANSMIT_INTERVAL) {
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
    turnLeft = (char)(leftBlinker.read());
    turnRight = (char)(rightBlinker.read());
    char wiperVal = (char)(wipers.read());

	// hazards on == both blinkers turned on at the same time
    if (hazardsVal) {
		turnLeft = 1;
        turnRight = 1;
    }

    char dataStr = (wiperVal << 6) | (turnLeft << 5) | (turnRight << 4) | (hazardsVal << 3) | (hornVal << 2) | (brakeVal << 1) | lightsVal.getValue();
    return dataStr;
}

void handle_motor_inputs(){
	if (timerMotor.read_ms() > MOTOR_CONTROLLER_TRANSMIT_INTERVAL) {

		dmsVal = getDmsVal();
		ignitionVal = (char)ignition.read();
		brakeVal = (char)!brake.read();

		currentThrottle = get_throttle_val();
		
		if (!dmsVal.getValue() || !ignitionVal.getValue() || brakeVal.getValue()) {
			currentThrottle = 0;
		}

		// Throttle Data
		const unsigned char throttleData[] = { currentThrottle.getValue() };
		can.write(CANMessage(CAN_STEERING_THROTTLE, throttleData, 1));

		// Ready Data
		char readyVal = (brakeVal.getValue() << 2) | (dmsVal.getValue() << 1) | ignitionVal.getValue();
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

void handle_reset_gesture() {
	
	char thisGesture = ignition.read();
	int thisGestureTime = timerDisplay.read_ms();
	if (g_lastGestureTime + DEBOUNCE_TIME < thisGestureTime && g_lastGesture != thisGesture) {

		if (thisGestureTime <= g_lastGestureTime + GESTURE_MARGIN) {
			g_lastTime = -1;
			g_lastGestureTime = 0;
			timerDisplay.reset();
		} else {
			g_lastGestureTime = thisGestureTime;
		}
	}
	g_lastGesture = thisGesture;
}