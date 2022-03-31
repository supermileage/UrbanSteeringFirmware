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
#include "graphics.h"
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
DigitalOut dmsLed(A5);

Timer timerMotor;
Timer timerDisplay;
Timer timerAccessories;
Timer timerFlash;

// globals variables shared between main and display threads
unsigned _currentSpeed = 0;
float _currentBmsSoc = 0.0f;
float _currentBmsVoltage = 0.0f;
uint8_t _currentThrottle = 0;
int _lastGestureTime = 0;
unsigned long _startTime = 0;
char _ignitionVal = 0;
char _dmsVal = 0;
char _brakeVal = 0;
char _lastGesture = 0;
char _turnLeft = 0;
char _turnRight = 0;
char _lights = 0;

int main() {
    timerMotor.start();
    timerDisplay.start();
    timerAccessories.start();
	char dummy;
    uint8_t prev_dataStr = read_accessory_inputs(dummy);
    uint8_t curr_dataStr = 0;
    CANMessage msg;
	_startTime = time(NULL);

	initialize_display();
	Thread display_thread;
	display_thread.start(display_task);

	dmsLed.write(1);

    while(1){
		// check for time-reset gesture
		listen_reset_gesture();

		// Handle motor controller
		if (timerMotor.read_ms() > MOTOR_CONTROLLER_TRANSMIT_INTERVAL) {
            handle_motor_inputs();
            timerMotor.reset();
        }

		// Handle accessories
        if (timerAccessories.read_ms() > ACCESSORIES_TRANSMIT_INTERVAL){
			char hazardsOn;
            curr_dataStr = read_accessory_inputs(hazardsOn);
            if ((curr_dataStr != prev_dataStr)) {
				// turn hazards off
				if (hazardsOn) {
					const unsigned char data[] = { 0x2, 0x4 << 1, 0x5 << 1 };
					can.write(CANMessage(CAN_ACC_OPERATION, data, 3));
					wait_us(1000);
				}
                const char data[] = {0, curr_dataStr};
                can.write(CANMessage(CAN_ACC_OPERATION, data, 2));
                prev_dataStr = curr_dataStr;
            }
            timerAccessories.reset();
        }

		// Check for gps speed / bms data updates from telemetry
		if (can.read(msg)) {
			if (msg.id == CAN_TELEMETRY_GPS_DATA) {
				_currentSpeed = (unsigned)msg.data[0];
			} else if (msg.id == CAN_TELEMETRY_BMS_DATA) {
				memcpy((void*)&_currentBmsSoc, (void*)msg.data, 4);
				memcpy((void*)&_currentBmsVoltage, (void*)(msg.data + 4), 4);
			}
		}
    }
}


char read_accessory_inputs(char& hazardsVal){
    _lights = (char)(lights.read());
	char brakeVal = (char)(!brake.read());
    char hornVal = (char)(horn.read());
    hazardsVal = (char)(hazards.read());
    _turnRight = (char)(rightBlinker.read());
    _turnLeft = (char)(leftBlinker.read());
    char wiperVal = (char)(wipers.read());

	// hazards on == both blinkers turned on at the same time
    if (hazardsVal) {
		_turnLeft = 1;
        _turnRight = 1;
    }

    char dataStr = (wiperVal << 6) | (_turnLeft << 5) | (_turnRight << 4) | (hazardsVal << 3) | (hornVal << 2) | (brakeVal << 1) | _lights;
    return dataStr;
}

void handle_motor_inputs(){
    _dmsVal = get_dms_val();
    _ignitionVal = (char)ignition.read();
    _brakeVal = (char)!brake.read();

    _currentThrottle = get_throttle_val();
    
    if (!_dmsVal || !_ignitionVal || _brakeVal) {
        _currentThrottle = 0;
    }

	// Throttle Data
	const unsigned char throttleData[] = { _currentThrottle };
    can.write(CANMessage(CAN_STEERING_THROTTLE, throttleData, 1));

	// Ready Data
    char readyVal = (_brakeVal << 2) | (_dmsVal << 1) | _ignitionVal;
    const char readyData[] = { readyVal };
    can.write(CANMessage(CAN_STEERING_READY, readyData, 1));
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
	float dms_ctrl = dms.read();
	dmsLed.write(0);
	wait_us(40);
    float dms_val = dms.read();
	dmsLed.write(1);

    return dms_val <= DMS_THRESH && (dms_val < dms_ctrl - DMS_DELTA);
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
#define SPEED_Y_LABEL 103
#define SPEED_X 80
#define SPEED_X_UNIT_OFFSET 110
#define SPEED_Y 85

// power
#define POWER_X_LABEL 48
#define POWER_Y_LABEL 153
#define POWER_X 80
#define POWER_X_UNIT_OFFSET	110
#define POWER_Y 135

// time
#define TIME_X_LABEL 	46
#define TIME_Y_LABEL 	203
#define MINUTES_X 		80
#define COLON_X			160
#define SECONDS_X 		185
#define TIME_Y 			185

// throttle debug
#define THROTTLE_RAW_X 10
#define THROTTLE_RAW_Y 210

// turn signals
#define TURN_WIDTH		30
#define TURN_HEIGHT		30
#define TURN_LEFT_X 	10
#define TURN_LEFT_Y		55
#define TURN_RIGHT_X 	280
#define TURN_RIGHT_Y	60

// lights
#define LIGHTS_X		140
#define LIGHTS_Y		55
#define LIGHTS_WIDTH	40
#define LIGHTS_HEIGHT	30


// display data cache
int _lastTime = 0;
unsigned _lastSpeed = 0;
uint8_t _lastThrottle = 0;
float _lastBmsSoc = 0;
float _lastBmsVoltage = 0;
unsigned _lastMinutes = 0;
unsigned _lastSeconds = 0;
char _lastDmsVal = 0;
char _lastIgnitionVal = 0;
char _lastBrakeVal = 0;
char _lastTurnLeft = 0;
char _lastTurnRight = 0;
char _lastLights = 0;

char _flash = 0;
char _lastFlash = 0;

void listen_reset_gesture() {
	char thisGesture = ignition.read();
	int thisGestureTime = timerDisplay.read_ms();
	if (_lastGestureTime + DEBOUNCE_TIME < thisGestureTime && _lastGesture != thisGesture) {

		if (thisGestureTime <= _lastGestureTime + GESTURE_MARGIN) {
			_lastTime = -1;
			_lastGestureTime = 0;
			timerDisplay.reset();
		} else {
			_lastGestureTime = thisGestureTime;
		}
	}
	_lastGesture = thisGesture;
}

void display_task() {
	while (true) {
		TFT.set_font((unsigned char*) SMALL_FONT);

		// Update DMS
		if (_lastDmsVal != _dmsVal) {
			if (_dmsVal) {
    			TFT.fillcircle(DMS_X + CIRCLE_X_OFFSET_DMS, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Green);
			} else {
    			TFT.fillcircle(DMS_X + CIRCLE_X_OFFSET_DMS, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Red);
			}
			_lastDmsVal = _dmsVal;
		}

		// Update Ignition
		if (_lastIgnitionVal != _ignitionVal) {
			if (_ignitionVal) {
				TFT.fillcircle(IGNITION_X + CIRCLE_X_OFFSET_IGNITION, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Green);
			} else {
				TFT.fillcircle(IGNITION_X + CIRCLE_X_OFFSET_IGNITION, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Red);
			}
			_lastIgnitionVal = _ignitionVal;
		}

		// Update Brake
		if (_lastBrakeVal != _brakeVal) {
			if (!_brakeVal) {
				TFT.fillcircle(BRAKE_X + CIRCLE_X_OFFSET_BRAKE, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Green);
			} else {
				TFT.fillcircle(BRAKE_X + CIRCLE_X_OFFSET_BRAKE, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Red);
			}
			_lastBrakeVal = _brakeVal;
		}

		// Update voltage
		if (_lastBmsVoltage != _currentBmsVoltage) {
			// update text
			const char* padding = _currentBmsVoltage < 10.0f ? "0" : "";
			TFT.locate(VOLTAGE_TEXT_X, VOLTAGE_TEXT_Y);
			TFT.printf("%s%.2f", padding, _currentBmsVoltage);
			_lastBmsVoltage = _currentBmsVoltage;

			// redraw rectangle
			// float voltage = _currentBmsVoltage > 0.0f ? _currentBmsVoltage : -_currentBmsVoltage; // might not be necessary, unless negative voltage is possible?
			int rectangleWidth = (int)(_currentBmsSoc / 100.0f * (float)VOLTAGE_WIDTH);
			TFT.fillrect(VOLTAGE_LEFT_X + VOLTAGE_PADDING, VOLTAGE_LEFT_Y + VOLTAGE_PADDING, VOLTAGE_LEFT_X - VOLTAGE_PADDING + rectangleWidth , VOLTAGE_RIGHT_Y - VOLTAGE_PADDING, Green);
			TFT.fillrect(VOLTAGE_LEFT_X + VOLTAGE_PADDING + rectangleWidth, VOLTAGE_LEFT_Y + VOLTAGE_PADDING, VOLTAGE_RIGHT_X - VOLTAGE_PADDING, VOLTAGE_RIGHT_Y - VOLTAGE_PADDING, Black);
		}

		// Set font size for large test display
		TFT.set_font((unsigned char*) COOL_FONT);

		// Update Speed
		if (_lastSpeed != _currentSpeed) {
			_lastSpeed = _currentSpeed;
			TFT.locate(SPEED_X, SPEED_Y);
			TFT.printf("%02u", _currentSpeed);
		}

		// Update Power
		if(_lastThrottle != _currentThrottle) {
			_lastThrottle = _currentThrottle;
			TFT.locate(POWER_X, POWER_Y);
			TFT.printf("%03u", (_currentThrottle * 100) / 255);
			
		}

		// Update Time
		int currentTime = timerDisplay.read_ms() / 1000;
		if (currentTime != _lastTime) {
			int minutes = currentTime / 60;
			int seconds = currentTime % 60;

			if(minutes != _lastMinutes){
				TFT.locate(MINUTES_X, TIME_Y);
				TFT.printf("%02u", minutes);
				_lastMinutes = minutes;
			}

			if(seconds != _lastSeconds){
				TFT.locate(SECONDS_X, TIME_Y);
				TFT.printf("%02u", seconds);
				_lastSeconds = seconds;
			}

			_lastTime = currentTime;
		}

		// Update Turn signals

		if(_flash && timerFlash.read_ms() % 1000 >= 500) {
			_flash = 0;
		}
		
		if(!_flash && timerFlash.read_ms() % 1000 < 500 && timerFlash.read_ms() != 0) {
			_flash = 1;
		}

		if(_turnLeft != _lastTurnLeft) {
			if(_turnLeft) {
				printf("LEFT TURN PRESSED\n");
				timerFlash.start();
				if(!_flash) {
					_flash = 1;
				}
			} else {
				if(!_turnRight) {
					_flash = 0;
					timerFlash.stop();
					timerFlash.reset();
				}
			}
			_lastTurnLeft = _turnLeft;
		}

		if(_turnRight != _lastTurnRight) {
			if(_turnRight) {
				timerFlash.start();
				if(!_flash) {
					_flash = 1;
				}
			} else {
				if(!_turnLeft) {
					_flash = 0;
					timerFlash.stop();
					timerFlash.reset();
				}
			}
			_lastTurnRight = _turnRight;
		}

		if(_flash != _lastFlash) {
			if(_flash) {
				printf("FLASH STARTING\n");
				if(_turnRight) {
					TFT.Bitmap(TURN_RIGHT_X, TURN_RIGHT_Y, TURN_WIDTH, TURN_HEIGHT, graphicRightArrow);
				}
				if(_turnLeft) {
					printf("FLASH STARTING - LEFT\n");
					TFT.Bitmap(TURN_LEFT_X, TURN_LEFT_Y, TURN_WIDTH, TURN_HEIGHT, graphicLeftArrow);
				}
			} else {
				TFT.fillrect(TURN_RIGHT_X, TURN_RIGHT_Y, TURN_RIGHT_X + TURN_WIDTH, TURN_RIGHT_Y + TURN_HEIGHT, Black);
				TFT.fillrect(TURN_LEFT_X, TURN_LEFT_Y, TURN_LEFT_X + TURN_WIDTH, TURN_LEFT_Y + TURN_HEIGHT, Black);
			}
			_lastFlash = _flash;
		}

		// Update Lights
		if(_lights != _lastLights) {
			if (_lights) {
				TFT.Bitmap(LIGHTS_X, LIGHTS_Y, LIGHTS_WIDTH, LIGHTS_HEIGHT, graphicLights);
			} else {
				TFT.fillrect(LIGHTS_X, LIGHTS_Y, LIGHTS_X + LIGHTS_WIDTH, LIGHTS_Y + LIGHTS_HEIGHT, Black);
			}
			_lastLights = _lights;
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