#include "stdio.h"
#include "mbed.h"
#include "rtos.h"
#include "SPI_TFT_ILI9341.h"
#include "Arial12x12.h"
#include "Arial28x28.h"
#include "supermileagelogo.h"
#include "can_common.h"
#include "main.h"

#include <string>
#include <cmath>

using namespace std;

#pragma region Macros

#define DEBUG_THROTTLE 1

#define TFT_DISPLAY 1
#define DMS_THRESH 0.1f
#define ACCESSORIES_TRANSMIT_INTERVAL 0.05f
#define MOTOR_CONTROLLER_TRANSMIT_INTERVAL 0.05f

#define DEBOUNCE_TIME 0.05f
#define GESTURE_MARGIN 0.5f
#define MIN_THROTTLE_OUTPUT 0.0f
#define MAX_THROTTLE_OUTPUT 255.0f
#define MIN_THROTTLE_INPUT 0.35f
#define MAX_THROTTLE_INPUT 0.455f
#define SLOPE (MAX_THROTTLE_OUTPUT - MIN_THROTTLE_OUTPUT) / (MAX_THROTTLE_INPUT - MIN_THROTTLE_INPUT)
#define OFFSET MAX_THROTTLE_OUTPUT - (SLOPE * MAX_THROTTLE_INPUT)

#pragma endregion

#pragma region Globals

// Compile resources for screen or serial port debugging
#if TFT_DISPLAY
	SPI_TFT_ILI9341 TFT(D11, D12, D13, A7, D0, A3, "TFT");
#else
	Serial pc(USBTX, USBRX); // tx, rx
#endif

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
unsigned _currentSpeed = 0;
float _currentBmsSoc = 0.0f;
float _currentBmsVoltage = 0.0f;
uint8_t _currentThrottle = 0;
float _lastGestureTime = 0.0f;
unsigned long _startTime = 0;
char _ignitionVal = 0;
char _dmsVal = 0;
char _brakeVal = 0;
char _lastGesture = 0;

#pragma endregion

int main() {
    timerMotor.start();
    timerDisplay.start();
    timerAccessories.start();
	char dummy;
    uint8_t prev_dataStr = read_accessory_inputs(dummy);
    uint8_t curr_dataStr = 0;
    CANMessage msg;
	_startTime = time(NULL);

	#if TFT_DISPLAY
		initialize_display();
		Thread display_thread;
		display_thread.start(display_task);
	#endif

    while(1){
		// check for time-reset gesture
		listen_reset_gesture();

		// Handle motor controller
		if (timerMotor.read() > MOTOR_CONTROLLER_TRANSMIT_INTERVAL) {
            handle_motor_inputs();
            timerMotor.reset();
        }

		// Handle accessories
        if (timerAccessories.read() > ACCESSORIES_TRANSMIT_INTERVAL){
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

#pragma region Can

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
    _dmsVal = get_dms_val();
    _ignitionVal = (char)ignition.read();
    _brakeVal = (char)brake.read();
	
    _currentThrottle = get_throttle_val();
    
    if (!_dmsVal || !_ignitionVal || !_brakeVal) {
        _currentThrottle = 0;
    }

	#if !TFT_DISPLAY
		pc.printf("Status -- DMS: %d\t Ignition: %d\n", _dmsVal, _ignitionVal);
	#endif

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
    float dms_val = dms.read();
    char dms_flag = 0;

	#if !TFT_DISPLAY
		pc.printf("DMS Read Value: %f\n", dms_val);
	#endif

    if (dms_val <= DMS_THRESH){
        dms_flag = 1;
    }
    else if (dms_val > DMS_THRESH){
        dms_flag = 0;
	}
    return dms_flag;
}

#pragma endregion

#pragma region Display

#if TFT_DISPLAY

// display macros

// accessories
#define DMS_X 10
#define IGNITION_X 60
#define BRAKE_X		100
#define STATUS_Y 5
#define CIRCLE_RADIUS 10
#define CIRCLE_Y_OFFSET CIRCLE_RADIUS * 3
#define CIRCLE_X_OFFSET_DMS  14
#define CIRCLE_X_OFFSET_IGNITION  10
#define CIRCLE_X_OFFSET_BRAKE  14

// voltage
#define VOLTAGE_TEXT_X 260
#define VOLTAGE_TEXT_Y STATUS_Y + 17
#define VOLTAGE_UNIT_OFFSET 41
#define VOLTAGE_LEFT_X 150
#define VOLTAGE_LEFT_Y 11
#define VOLTAGE_WIDTH 100
#define VOLTAGE_HEIGHT 30
#define VOLTAGE_RIGHT_X VOLTAGE_LEFT_X + VOLTAGE_WIDTH
#define VOLTAGE_RIGHT_Y VOLTAGE_LEFT_Y + VOLTAGE_HEIGHT
#define VOLTAGE_PADDING 3

#define VOLTAGE_BTN_X 250
#define VOLTAGE_BTN_Y 20
#define VOLTAGE_BTN_WIDTH 2
#define VOLTAGE_BTN_HEIGHT 10

// speed
#define SPEED_X 100
#define SPEED_Y 80
#define SPEED_UNIT_OFFSET 60
#define LARGE_FONT Arial28x28
#define SMALL_FONT Arial12x12

// power
#define POWER_X 60
#define POWER_X_OFFSET 90
#define POWER_X_UNIT_OFFSET	150
#define POWER_Y 130

// time
#define MINUTES_X 105
#define MINUTES_OFFSET 25
#define COLON_X 150
#define SECONDS_X 170
#define TIME_Y 180



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

void listen_reset_gesture() {
	char thisGesture = ignition.read();
	float thisGestureTime = timerDisplay.read();
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
			if (_brakeVal) {
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
		TFT.set_font((unsigned char*) LARGE_FONT);

		// Update Speed
		if (_lastSpeed != _currentSpeed) {
			TFT.locate(SPEED_X, SPEED_Y);
			TFT.printf("%u", _currentSpeed);
			_lastSpeed = _currentSpeed;
		}

		// Update Power

		if(_lastThrottle != _currentThrottle) {
			TFT.locate(POWER_X + POWER_X_OFFSET, POWER_Y);
			TFT.printf("%u", (_currentThrottle * 100) / 255);
			_lastThrottle = _currentThrottle;
		}

		// Update Time
		int currentTime = static_cast<int>(timerDisplay.read());
		if (currentTime > _lastTime) {
			int minutes = static_cast<int>(currentTime) / 60;
			int seconds = static_cast<int>(currentTime) % 60;

			if (_lastMinutes ^ minutes) {
				const char* padding = minutes < 10 ? "0" : "";
				int minutes_x = minutes < 100 ? MINUTES_X : MINUTES_X - MINUTES_OFFSET;
				TFT.locate(minutes_x, TIME_Y);
				TFT.printf("%s%u", padding, minutes);
				_lastMinutes = minutes;
			}

			if (_lastSeconds ^ seconds) {
				const char* padding = seconds < 10 ? "0" : "";
				TFT.locate(SECONDS_X, TIME_Y);
				TFT.printf("%s%u", padding, seconds);
				_lastSeconds = seconds;
			}
		}

		#if DEBUG_THROTTLE
		float throttleReadings = throttle.read();
		unsigned char throttleVal = get_throttle_val();
		string padding = "";
		if (throttleVal < 10) {
			padding = "00";
		} else if (throttleVal < 100) {
			padding = "0";
		}
		TFT.locate(SPEED_X, SPEED_Y);
		TFT.printf("%.4f", throttleReadings);
		TFT.locate(SPEED_X, SPEED_Y + 40);
		TFT.printf("%s%u", padding.c_str(), throttleVal);
		#else
		// Update Speed
		if (_lastSpeed != _currentSpeed) {
			TFT.locate(SPEED_X, SPEED_Y);
			TFT.printf("%u", _currentSpeed);
			_lastSpeed = _currentSpeed;
		}
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
	TFT.fillcircle(BRAKE_X + CIRCLE_X_OFFSET_BRAKE, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Red);
	// voltage
	TFT.locate(VOLTAGE_TEXT_X, VOLTAGE_TEXT_Y);
	TFT.printf("00.00");
	TFT.locate(VOLTAGE_TEXT_X + VOLTAGE_UNIT_OFFSET, VOLTAGE_TEXT_Y);
	TFT.printf("V");
	TFT.rect(VOLTAGE_LEFT_X, VOLTAGE_LEFT_Y, VOLTAGE_LEFT_X + VOLTAGE_WIDTH, VOLTAGE_LEFT_Y + VOLTAGE_HEIGHT, White);
	TFT.fillrect(VOLTAGE_LEFT_X + VOLTAGE_PADDING, VOLTAGE_LEFT_Y + VOLTAGE_PADDING, VOLTAGE_RIGHT_X - VOLTAGE_PADDING, VOLTAGE_RIGHT_Y - VOLTAGE_PADDING, Green);
	TFT.rect(VOLTAGE_BTN_X, VOLTAGE_BTN_Y, VOLTAGE_BTN_X + VOLTAGE_BTN_WIDTH, VOLTAGE_BTN_Y + VOLTAGE_BTN_HEIGHT, White);

	// Large font values
	TFT.set_font((unsigned char*) LARGE_FONT);
	// speed
	TFT.locate(SPEED_X, SPEED_Y);
	TFT.printf("0");
	TFT.locate(SPEED_X + SPEED_UNIT_OFFSET, SPEED_Y);
	TFT.printf("km/h");
	// power
	TFT.locate(POWER_X, POWER_Y);
	TFT.printf("PWR:");
	TFT.locate(POWER_X + POWER_X_OFFSET, POWER_Y);
	TFT.printf("0");
	TFT.locate(POWER_X + POWER_X_UNIT_OFFSET, POWER_Y);
	TFT.printf("%");
	// time
	TFT.locate(MINUTES_X, TIME_Y);
	TFT.printf("00");
	TFT.locate(COLON_X, TIME_Y);
	TFT.printf(":");
	TFT.locate(SECONDS_X, TIME_Y);
	TFT.printf("00");
	
	#if DEBUG_THROTTLE
	// don't draw speed
	#else
	// speed
	TFT.locate(SPEED_X, SPEED_Y);
	TFT.printf("0");
	TFT.locate(SPEED_X + SPEED_UNIT_OFFSET, SPEED_Y);
	TFT.printf("km/h");
	#endif

}

#endif

#pragma endregion