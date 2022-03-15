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

using namespace std;

#pragma region Macros

#define TFT_DISPLAY 1
#define DMS_THRESH 0.1f
#define ACCESSORIES_TRANSMIT_INTERVAL 0.2f
#define MOTOR_CONTROLLER_TRANSMIT_INTERVAL 0.05f

#define MIN_THROTTLE_OUTPUT 0.0f
#define MAX_THROTTLE_OUTPUT 255.0f
#define MIN_THROTTLE_INPUT 0.28f
#define MAX_THROTTLE_INPUT 0.425f
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
DigitalIn brake(D1,PullDown);
DigitalIn horn(D5,PullDown);
DigitalIn hazards(D9,PullDown);
DigitalIn rightBlinker(D4,PullDown);
DigitalIn leftBlinker(D3,PullDown);
DigitalIn wipers(A2,PullDown);

// Ready
DigitalIn ignition(D6,PullDown);
AnalogIn dms(A1);
AnalogIn throttle(A6);

Timer timer_MTR;
Timer timer_TEL;
Timer timer_ACC;

// globals shared between main and display threads
unsigned _currentSpeed = 0;
float _currentBmsSoc = 0.0f;
float _currentBmsVoltage = 0.0f;
unsigned long _startTime = 0;
char _ignitionVal = 0;
char _dmsVal = 0;

#pragma endregion

int main() {
    timer_MTR.start();
    timer_TEL.start();
    timer_ACC.start();
    uint8_t prev_dataStr = ACC_task();
    uint8_t curr_dataStr = 0;
    CANMessage msg;
	_startTime = time(NULL);

	#if TFT_DISPLAY
		initialize_display();
		Thread display_thread(display_task);
	#endif

    while(1){
		// Handle motor controller
		if (timer_MTR.read() > MOTOR_CONTROLLER_TRANSMIT_INTERVAL) {
            MTR_task();
            timer_MTR.reset();
        }

		// Handle accessories
        if (timer_ACC.read() > ACCESSORIES_TRANSMIT_INTERVAL){
            curr_dataStr = ACC_task();
            if ((curr_dataStr ^ prev_dataStr)) {
                const char data[] = {0, curr_dataStr};
                can.write(CANMessage(CAN_ACC_OPERATION, data, 2));
                prev_dataStr = curr_dataStr;
            }
            timer_ACC.reset();
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

char ACC_task(){
    char lightsVal = (char)(lights.read());
	char brakeVal = (char)(!brake.read());
    char hornVal = (char)(horn.read());
    char hazardVal = (char)(hazards.read());
    char rightbVal = (char)(rightBlinker.read());
    char leftbVal = (char)(leftBlinker.read());
    char wiperVal = (char)(wipers.read());

    if (hazardVal) {
        leftbVal = 0;
        rightbVal = 0;
    }

    char dataStr = (wiperVal << 6) | (leftbVal << 5) | (rightbVal << 4) | (hazardVal << 3) | (hornVal << 2) | (brakeVal << 1) | lightsVal;
    return dataStr;
}

void MTR_task(){
    _dmsVal = get_dms_val();
    _ignitionVal = (char)ignition.read();
    char brakeVal = (char)!brake.read();

    unsigned char throttleVal = 0;
    
    if (_dmsVal && _ignitionVal && !brakeVal) {
        throttleVal = get_throttle_val();
    }

	#if !TFT_DISPLAY
		pc.printf("Status -- DMS: %d\t Ignition: %d\n", _dmsVal, _ignitionVal);
	#endif

	// Throttle Data
	const unsigned char throttleData[] = { throttleVal };
    can.write(CANMessage(CAN_STEERING_THROTTLE, throttleData, 1));

	// Ready Data
    char readyVal = (brakeVal << 2) | (_dmsVal << 1) | _ignitionVal;
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
#define DMS_X 17
#define IGNITION_X 83
#define STATUS_Y 5
#define CIRCLE_RADIUS 10
#define CIRCLE_Y_OFFSET CIRCLE_RADIUS * 3
#define CIRCLE_X_OFFSET_DMS  14
#define CIRCLE_X_OFFSET_IGNITION  10

// voltage
#define VOLTAGE_TEXT_X 258
#define VOLTAGE_TEXT_Y STATUS_Y + 17
#define VOLTAGE_UNIT_OFFSET 41
#define VOLTAGE_LEFT_X 150
#define VOLTAGE_LEFT_Y 11
#define VOLTAGE_WIDTH 100
#define VOLTAGE_HEIGHT 30
#define VOLTAGE_RIGHT_X VOLTAGE_LEFT_X + VOLTAGE_WIDTH
#define VOLTAGE_RIGHT_Y VOLTAGE_LEFT_Y + VOLTAGE_HEIGHT
#define VOLTAGE_PADDING 3

// time
#define MINUTES_X 100
#define MINUTES_OFFSET 25
#define COLON_X 150
#define SECONDS_X 175
#define TIME_Y 100

// speed
#define SPEED_X 100
#define SPEED_Y 165
#define SPEED_UNIT_OFFSET 60
#define LARGE_FONT Arial28x28
#define SMALL_FONT Arial12x12

// display data cache
unsigned long _lastTime = 0;
unsigned _lastSpeed = 0;
float _lastBmsSoc = 0.0f;
float _lastBmsVoltage = 0.0f;
unsigned _lastMinutes = 0;
unsigned _lastSeconds = 0;
char _lastDmsVal = 0;
char _lastIgnitionVal = 0;

void display_task(const void* arg) {
	while (true) {
		unsigned long currentTime = time(NULL);
		TFT.set_font((unsigned char*) SMALL_FONT);

		// Update DMS
		if (_lastDmsVal ^ _dmsVal) {
			if (_dmsVal) {
    			TFT.fillcircle(DMS_X + CIRCLE_X_OFFSET_DMS, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Green);
			} else {
    			TFT.fillcircle(DMS_X + CIRCLE_X_OFFSET_DMS, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Red);
			}
			_lastDmsVal = _dmsVal;
		}
		// Update Ignition
		if (_lastIgnitionVal ^ _ignitionVal) {
			if (_ignitionVal) {
				TFT.fillcircle(IGNITION_X + CIRCLE_X_OFFSET_IGNITION, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Green);
			} else {
				TFT.fillcircle(IGNITION_X + CIRCLE_X_OFFSET_IGNITION, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Red);
			}
			_lastIgnitionVal = _ignitionVal;
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
		wait(0.125);

		TFT.set_font((unsigned char*) LARGE_FONT);
		// Update Time
		if (_lastTime ^ currentTime) {
			unsigned minutes = currentTime / 60;
			unsigned seconds = currentTime % 60;

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
		// Update Speed
		if (_lastSpeed ^ _currentSpeed) {
			TFT.locate(SPEED_X, SPEED_Y);
			TFT.printf("%u", _currentSpeed);
			_lastSpeed = _currentSpeed;
		}
		wait(0.125);
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
	// voltage
	TFT.locate(VOLTAGE_TEXT_X, VOLTAGE_TEXT_Y);
	TFT.printf("00.00");
	TFT.locate(VOLTAGE_TEXT_X + VOLTAGE_UNIT_OFFSET, VOLTAGE_TEXT_Y);
	TFT.printf("V");
	TFT.rect(VOLTAGE_LEFT_X, VOLTAGE_LEFT_Y, VOLTAGE_LEFT_X + VOLTAGE_WIDTH, VOLTAGE_LEFT_Y + VOLTAGE_HEIGHT, White);
	TFT.fillrect(VOLTAGE_LEFT_X + VOLTAGE_PADDING, VOLTAGE_LEFT_Y + VOLTAGE_PADDING, VOLTAGE_RIGHT_X - VOLTAGE_PADDING, VOLTAGE_RIGHT_Y - VOLTAGE_PADDING, Green);
	
	// Large font values
	TFT.set_font((unsigned char*) LARGE_FONT);
	// time
	TFT.locate(MINUTES_X, TIME_Y);
	TFT.printf("00");
	TFT.locate(COLON_X, TIME_Y);
	TFT.printf(":");
	TFT.locate(SECONDS_X, TIME_Y);
	TFT.printf("00");
	// speed
	TFT.locate(SPEED_X, SPEED_Y);
	TFT.printf("0");
	TFT.locate(SPEED_X + SPEED_UNIT_OFFSET, SPEED_Y);
	TFT.printf("km/h");

}

#endif

#pragma endregion