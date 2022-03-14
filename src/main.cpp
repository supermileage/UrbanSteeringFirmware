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

CAN can(D10, D2, 500000);

DigitalIn lights(A0,PullDown); // 
DigitalIn brake(D1,PullDown); // 
DigitalIn horn(D5,PullDown); // PB6
DigitalIn hazards(D9,PullDown); // PA8
DigitalIn rightBlinker(D4,PullDown); //PB7
DigitalIn leftBlinker(D3,PullDown); //PB0
DigitalIn wipers(A2,PullDown); // PA3
DigitalIn ignition(D6,PullDown);
AnalogIn dms(A1); //deadman switch
AnalogIn throttle(A6);

#if TFT_DISPLAY
	// LCD Display
	SPI_TFT_ILI9341 TFT(D11, D12, D13, A7, D0, A3, "TFT");
#else
	// Serial Out
	Serial pc(USBTX, USBRX); // tx, rx
#endif

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
    uint8_t prev_data_str = ACC_task();
    uint8_t curr_data_str = 0;
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
            curr_data_str = ACC_task();
            if ((curr_data_str ^ prev_data_str)) {
                const char data[] = {0, curr_data_str};
                can.write(CANMessage(CAN_ACC_OPERATION, data, 2));
                prev_data_str = curr_data_str;
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
	char brake_val = (char)(!brake.read());
    char hornVal = (char)(horn.read());
    char hazardVal = (char)(hazards.read());
    char rightbVal = (char)(rightBlinker.read());
    char leftbVal = (char)(leftBlinker.read());
    char wiperVal = (char)(wipers.read());

    if (hazardVal) {
        leftbVal = 0;
        rightbVal = 0;
    }

    char data_str = (wiperVal << 6) | (leftbVal << 5) | (rightbVal << 4) | (hazardVal << 3) | (hornVal << 2) | (brake_val << 1) | lightsVal;
    return data_str;
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
    char brake_val = (char)!brake.read();

    unsigned char throttle_val = 0;
    
    if (_dmsVal && _ignitionVal && !brake_val) {
        throttle_val = get_throttle_val();
    }

	#if !TFT_DISPLAY
		pc.printf("Status -- DMS: %d\t Ignition: %d\n", _dmsVal, _ignitionVal);
	#endif

	// Throttle Data
	const unsigned char throttle_data[] = { throttle_val };
    can.write(CANMessage(CAN_STEERING_THROTTLE, throttle_data, 1));

	// Ready Data
    char ready_val = (brake_val << 2) | (_dmsVal << 1) | _ignitionVal;
    const char ready_data[] = { ready_val };
    can.write(CANMessage(CAN_STEERING_READY, ready_data, 1));
}

unsigned char get_throttle_val() {
	float throttle_as_float = throttle.read();

	if (throttle_as_float <= MIN_THROTTLE_INPUT) {
		return (unsigned char)MIN_THROTTLE_OUTPUT;
	} else if (throttle_as_float >= MAX_THROTTLE_INPUT) {
		return (unsigned char)MAX_THROTTLE_OUTPUT;
	}

	return (unsigned char)(throttle_as_float * SLOPE + OFFSET);
}

#if TFT_DISPLAY

void display_task(const void* arg) {
	while (true) {
			unsigned long current_time = time(NULL);
			unsigned minutes = current_time / 60;
			unsigned seconds = current_time % 60;
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