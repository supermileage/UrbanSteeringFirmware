#include <string>
#include <cmath>

#include "stdio.h"
#include "mbed.h"
#include "rtos.h"

#include "can_common.h"

#include "SPI_TFT_ILI9341.h"
#include "Arial12x12.h"
#include "Arial28x28.h"
#include "font_big.h"
#include "supermileagelogo.h"
#include "main.h"

using namespace std;

#define DEBUG_THROTTLE 0

#define DMS_THRESH 0.1f
#define ACCESSORIES_TRANSMIT_INTERVAL 50
#define MOTOR_CONTROLLER_TRANSMIT_INTERVAL 50

#define DEBOUNCE_TIME 50
#define GESTURE_MARGIN 500
#define MIN_THROTTLE_OUTPUT 0.0f
#define MAX_THROTTLE_OUTPUT 255.0f
#define MIN_THROTTLE_INPUT 0.35f
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

Timer timerMotor;
Timer timerDisplay;
Timer timerAccessories;

// globals variables shared between main and display threads
unsigned g_currentSpeed = 0;
float g_currentBmsSoc = 0.0f;
float g_currentBmsVoltage = 0.0f;
uint8_t g_currentThrottle = 0;
int g_lastGestureTime = 0;
char g_prevAccVal = 0;
char g_ignitionVal = 0;
char g_dmsVal = 0;
char g_brakeVal = 0;
char g_lastGesture = 0;

int main() {
    timerMotor.start();
    timerDisplay.start();
    timerAccessories.start();

	// Initalize Accessories--it's impossible for left and right blinker to be on simultaneously so this will cause can update to be sent on boot
    g_prevAccVal = 0xFF;

	initialize_display();
	Thread display_thread;
	display_thread.start(display_task);

    while(1){
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
		if ((currentAccVal != g_prevAccVal)) {
			// turn hazards off
			if (hazardsOn) {
				const unsigned char data[] = { 0x2, 0x4 << 1, 0x5 << 1 };
				can.write(CANMessage(CAN_ACC_OPERATION, data, 3));
				wait_us(1000);
			}
			const char data[] = {0, currentAccVal};
			can.write(CANMessage(CAN_ACC_OPERATION, data, 2));
			g_prevAccVal = currentAccVal;
		}
		timerAccessories.reset();
	}
}

char read_accessory_inputs(char& hazardsVal){
    char lightsVal = (char)(lights.read());
	char brakeVal = (char)(!brake.read());
    char hornVal = (char)(horn.read());
    hazardsVal = (char)(hazards.read());
    char rightbVal = (char)(rightBlinker.read());
    char leftbVal = (char)(leftBlinker.read());
    char wiperVal = (char)(wipers.read());

	// hazards on == both blinkers turned on at the same time
    if (hazardsVal) {
		leftbVal = 1;
        rightbVal = 1;
    }

    char dataStr = (wiperVal << 6) | (leftbVal << 5) | (rightbVal << 4) | (hazardsVal << 3) | (hornVal << 2) | (brakeVal << 1) | lightsVal;
    return dataStr;
}

void handle_motor_inputs(){
	if (timerMotor.read_ms() > MOTOR_CONTROLLER_TRANSMIT_INTERVAL) {

		g_dmsVal = get_dms_val();
		g_ignitionVal = (char)ignition.read();
		g_brakeVal = (char)!brake.read();

		g_currentThrottle = get_throttle_val();
		
		if (!g_dmsVal || !g_ignitionVal || g_brakeVal) {
			g_currentThrottle = 0;
		}

		// Throttle Data
		const unsigned char throttleData[] = { g_currentThrottle };
		can.write(CANMessage(CAN_STEERING_THROTTLE, throttleData, 1));

		// Ready Data
		char readyVal = (g_brakeVal << 2) | (g_dmsVal << 1) | g_ignitionVal;
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

char get_dms_val() {
    float dms_val = dms.read();
    char dms_flag = 0;

    if (dms_val <= DMS_THRESH){
        dms_flag = 1;
    }
    else if (dms_val > DMS_THRESH){
        dms_flag = 0;
	}
    return dms_flag;
}

// Checks for gps speed / bms data updates from telemetry
void receive_can() {
    CANMessage msg;

	if (can.read(msg)) {
		if (msg.id == CAN_TELEMETRY_GPS_DATA) {
			g_currentSpeed = (unsigned)msg.data[0];
		} else if (msg.id == CAN_TELEMETRY_BMS_DATA) {
			memcpy((void*)&g_currentBmsSoc, (void*)msg.data, 4);
			memcpy((void*)&g_currentBmsVoltage, (void*)(msg.data + 4), 4);
		}
	}
}


// display macros

// fonts 
#define SMALL_FONT 	Arial12x12
#define LARGE_FONT 	Arial28x28
#define COOL_FONT 	Neu42x35

// accessories
#define DMS_X 						10
#define IGNITION_X 					60
#define BRAKE_X						100
#define STATUS_Y 					5
#define CIRCLE_RADIUS 				10
#define CIRCLE_Y_OFFSET 			CIRCLE_RADIUS * 3
#define CIRCLE_X_OFFSET_DMS  		14
#define CIRCLE_X_OFFSET_IGNITION  	10
#define CIRCLE_X_OFFSET_BRAKE  		14

// voltage
#define VOLTAGE_TEXT_X 		260
#define VOLTAGE_TEXT_Y 		STATUS_Y + 17
#define VOLTAGE_UNIT_OFFSET 41
#define VOLTAGE_LEFT_X 		150
#define VOLTAGE_LEFT_Y 		11
#define VOLTAGE_WIDTH 		100
#define VOLTAGE_HEIGHT 		30
#define VOLTAGE_RIGHT_X 	VOLTAGE_LEFT_X + VOLTAGE_WIDTH
#define VOLTAGE_RIGHT_Y 	VOLTAGE_LEFT_Y + VOLTAGE_HEIGHT
#define VOLTAGE_PADDING 	3

#define VOLTAGE_BTN_X 		250
#define VOLTAGE_BTN_Y 		20
#define VOLTAGE_BTN_WIDTH 	2
#define VOLTAGE_BTN_HEIGHT 	10

// speed
#define SPEED_X_LABEL 50
#define SPEED_Y_LABEL 98
#define SPEED_X 80
#define SPEED_X_UNIT_OFFSET 110
#define SPEED_Y 80

// power
#define POWER_X_LABEL 48
#define POWER_Y_LABEL 148
#define POWER_X 80
#define POWER_X_UNIT_OFFSET	110
#define POWER_Y 130

// time
#define TIME_X_LABEL 	46
#define TIME_Y_LABEL 	198
#define MINUTES_X 		80
#define COLON_X			150
#define SECONDS_X 		165
#define TIME_Y 			180

// throttle debug
#define THROTTLE_RAW_X 10
#define THROTTLE_RAW_Y 210


// display data cache
int g_lastTime = 0;
unsigned g_lastSpeed = 0;
uint8_t g_lastThrottle = 0;
float g_lastBmsSoc = 0;
float g_lastBmsVoltage = 0;
unsigned g_lastMinutes = 0;
unsigned g_lastSeconds = 0;
char g_lastDmsVal = 0;
char g_lastIgnitionVal = 0;
char g_lastBrakeVal = 0;

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

void display_task() {
	while (true) {
		TFT.set_font((unsigned char*) SMALL_FONT);

		// Update DMS
		if (g_lastDmsVal != g_dmsVal) {
			if (g_dmsVal) {
    			TFT.fillcircle(DMS_X + CIRCLE_X_OFFSET_DMS, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Green);
			} else {
    			TFT.fillcircle(DMS_X + CIRCLE_X_OFFSET_DMS, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Red);
			}
			g_lastDmsVal = g_dmsVal;
		}

		// Update Ignition
		if (g_lastIgnitionVal != g_ignitionVal) {
			if (g_ignitionVal) {
				TFT.fillcircle(IGNITION_X + CIRCLE_X_OFFSET_IGNITION, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Green);
			} else {
				TFT.fillcircle(IGNITION_X + CIRCLE_X_OFFSET_IGNITION, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Red);
			}
			g_lastIgnitionVal = g_ignitionVal;
		}

		// Update Brake
		if (g_lastBrakeVal != g_brakeVal) {
			if (!g_brakeVal) {
				TFT.fillcircle(BRAKE_X + CIRCLE_X_OFFSET_BRAKE, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Green);
			} else {
				TFT.fillcircle(BRAKE_X + CIRCLE_X_OFFSET_BRAKE, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Red);
			}
			g_lastBrakeVal = g_brakeVal;
		}

		// Update voltage
		if (g_lastBmsVoltage != g_currentBmsVoltage) {
			// update text
			const char* padding = g_currentBmsVoltage < 10.0f ? "0" : "";
			TFT.locate(VOLTAGE_TEXT_X, VOLTAGE_TEXT_Y);
			TFT.printf("%s%.2f", padding, g_currentBmsVoltage);
			g_lastBmsVoltage = g_currentBmsVoltage;

			// redraw rectangle
			// float voltage = g_currentBmsVoltage > 0.0f ? g_currentBmsVoltage : -_currentBmsVoltage; // might not be necessary, unless negative voltage is possible?
			int rectangleWidth = (int)(g_currentBmsSoc / 100.0f * (float)VOLTAGE_WIDTH);
			TFT.fillrect(VOLTAGE_LEFT_X + VOLTAGE_PADDING, VOLTAGE_LEFT_Y + VOLTAGE_PADDING, VOLTAGE_LEFT_X - VOLTAGE_PADDING + rectangleWidth , VOLTAGE_RIGHT_Y - VOLTAGE_PADDING, Green);
			TFT.fillrect(VOLTAGE_LEFT_X + VOLTAGE_PADDING + rectangleWidth, VOLTAGE_LEFT_Y + VOLTAGE_PADDING, VOLTAGE_RIGHT_X - VOLTAGE_PADDING, VOLTAGE_RIGHT_Y - VOLTAGE_PADDING, Black);
		}

		// Set font size for large test display
		TFT.set_font((unsigned char*) COOL_FONT);

		// Update Speed
		if (g_lastSpeed != g_currentSpeed) {
			g_lastSpeed = g_currentSpeed;
			TFT.locate(SPEED_X, SPEED_Y);
			TFT.printf("%02u", g_currentSpeed);
		}

		// Update Power
		if(g_lastThrottle != g_currentThrottle) {
			g_lastThrottle = g_currentThrottle;
			TFT.locate(POWER_X, POWER_Y);
			TFT.printf("%03u", (g_currentThrottle * 100) / 255);
			
		}

		// Update Time
		int currentTime = timerDisplay.read_ms() / 1000;
		if (currentTime != g_lastTime) {
			int minutes = currentTime / 60;
			int seconds = currentTime % 60;

			if(minutes != g_lastMinutes){
				TFT.locate(MINUTES_X, TIME_Y);
				TFT.printf("%02u", minutes);
				g_lastMinutes = minutes;
			}

			if(seconds != g_lastSeconds){
				TFT.locate(SECONDS_X, TIME_Y);
				TFT.printf("%02u", seconds);
				g_lastSeconds = seconds;
			}

			g_lastTime = currentTime;
		}

		// Set font size for large test display
		TFT.set_font((unsigned char*) LARGE_FONT);

		#if DEBUG_THROTTLE

			TFT.locate(THROTTLE_RAW_X, THROTTLE_RAW_Y);
			TFT.printf("%4u", (unsigned int)(throttle.read() * 10000));

		#endif
	}
}

void initialize_display(){
	TFT.set_orientation(3);
	TFT.background(Black);
	TFT.cls();
	
	// Initialize Display Values
	TFT.set_font((unsigned char*) SMALL_FONT);
	// dms
	TFT.locate(DMS_X, STATUS_Y);
	TFT.printf("DMS");
    TFT.fillcircle(DMS_X + CIRCLE_X_OFFSET_DMS, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Red);
	// ignition
	TFT.locate(IGNITION_X, STATUS_Y);
	TFT.printf("IGN");
	TFT.fillcircle(IGNITION_X + CIRCLE_X_OFFSET_IGNITION, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Red);
	// brake
	TFT.locate(BRAKE_X, STATUS_Y);
	TFT.printf("BRK");
	TFT.fillcircle(BRAKE_X + CIRCLE_X_OFFSET_BRAKE, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Green);
	// voltage
	TFT.locate(VOLTAGE_TEXT_X, VOLTAGE_TEXT_Y);
	TFT.printf("00.00");
	TFT.locate(VOLTAGE_TEXT_X + VOLTAGE_UNIT_OFFSET, VOLTAGE_TEXT_Y);
	TFT.printf("V");
	TFT.rect(VOLTAGE_LEFT_X, VOLTAGE_LEFT_Y, VOLTAGE_LEFT_X + VOLTAGE_WIDTH, VOLTAGE_LEFT_Y + VOLTAGE_HEIGHT, White);
	TFT.fillrect(VOLTAGE_LEFT_X + VOLTAGE_PADDING, VOLTAGE_LEFT_Y + VOLTAGE_PADDING, VOLTAGE_RIGHT_X - VOLTAGE_PADDING, VOLTAGE_RIGHT_Y - VOLTAGE_PADDING, Green);
	TFT.rect(VOLTAGE_BTN_X, VOLTAGE_BTN_Y, VOLTAGE_BTN_X + VOLTAGE_BTN_WIDTH, VOLTAGE_BTN_Y + VOLTAGE_BTN_HEIGHT, White);

	// Labels
	TFT.locate(SPEED_X_LABEL, SPEED_Y_LABEL);
	TFT.printf("SPD");
	TFT.locate(POWER_X_LABEL, POWER_Y_LABEL);
	TFT.printf("PWR");
	TFT.locate(TIME_X_LABEL, TIME_Y_LABEL);
	TFT.printf("TIME");

	TFT.set_font((unsigned char*) COOL_FONT);

	// speed
	TFT.locate(SPEED_X, SPEED_Y);
	TFT.printf("00");
	TFT.locate(SPEED_X + SPEED_X_UNIT_OFFSET, SPEED_Y);
	TFT.printf("km/h");

	// power
	TFT.locate(POWER_X, POWER_Y);
	TFT.printf("000");
	TFT.locate(POWER_X + POWER_X_UNIT_OFFSET, POWER_Y);
	TFT.printf("%");

	// time
	TFT.locate(MINUTES_X, TIME_Y);
	TFT.printf("00");
	TFT.locate(COLON_X, TIME_Y);
	TFT.printf(":");
	TFT.locate(SECONDS_X, TIME_Y);
	TFT.printf("00");

	// Large font values
	TFT.set_font((unsigned char*) LARGE_FONT);

	#if DEBUG_THROTTLE
		TFT.locate(THROTTLE_RAW_X, THROTTLE_RAW_Y);
		TFT.printf("0000");
	#endif

}