#include "stdio.h"
#include "mbed.h"
#include "rtos.h"
#include "SPI_TFT_ILI9341.h"
#include "Arial28x28.h"
#include "supermileagelogo.h"
#include "can_common.h"
#include "main.h"

#include <string>  

#define TFT_DISPLAY 1
#define DMS_THRESH 0.1f
#define ACCESSORIES_TRANSMIT_INTERVAL 0.2f
#define MOTOR_CONTROLLER_TRANSMIT_INTERVAL 0.5f

#define MIN_THROTTLE_OUTPUT 0.0f
#define MAX_THROTTLE_OUTPUT 255.0f
#define MIN_THROTTLE_INPUT 0.28f
#define MAX_THROTTLE_INPUT 0.425f
#define SLOPE (MAX_THROTTLE_OUTPUT - MIN_THROTTLE_OUTPUT) / (MAX_THROTTLE_INPUT - MIN_THROTTLE_INPUT)
#define OFFSET MAX_THROTTLE_OUTPUT - (SLOPE * MAX_THROTTLE_INPUT)

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

// globals for use in TFT display
unsigned _currentSpeed = 0;
unsigned long _startTime = 0;
char _ignitionVal = 0;
char _dmsVal = 0;

using namespace std;

// Send Throttle As Zero If:
// 	Brake is on, deadman's switch is off, ignition is off
//  
//	Single byte data for telemetry
// ON SCREEN: gps speed and timer (minutes and seconds)

int main() {
    timer_MTR.start();
    timer_TEL.start();
    timer_ACC.start();
    uint8_t prev_dataStr = ACC_task();
    uint8_t curr_dataStr = 0;
    CANMessage msg;
	_startTime = time(NULL);

	#if TFT_DISPLAY
		set_up_display();
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

		// Check for gps speed updates from telemetry
		if (can.read(msg)) {
			if (msg.id == CAN_TELEMETRY_GPS_SPEED)
				_currentSpeed = (unsigned)msg.data[0];
		}
    }
}

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

#if TFT_DISPLAY

void display_task(const void* arg) {
	while (true) {
			unsigned long currentTime = time(NULL);
			unsigned minutes = currentTime / 60;
			unsigned seconds = currentTime % 60;
			const char* x = minutes < 10 ? "0" : "";
			const char* y = seconds < 10 ? "0" : "";

			TFT.locate(15, 15);
			TFT.printf("dms: %s", _dmsVal ? "on" : "off");
			TFT.locate(190, 15);
			TFT.printf("ign: %s", _ignitionVal ? "on" : "off");
			TFT.locate(100, 100);
			TFT.printf("%s%u : %s%u", x, minutes, y, seconds);
			TFT.locate(100, 165);
			TFT.printf("%ukm/h", _currentSpeed);
			wait(0.25);
	}
}

void set_up_display(){
	TFT.set_orientation(3);
	TFT.background(Black);
	TFT.cls();
	TFT.set_font((unsigned char*) Arial28x28);
	// print supermileage logo
	// TFT.Bitmap(55,78,201,85,supermileagelogo);
}

#endif