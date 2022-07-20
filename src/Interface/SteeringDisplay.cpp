#include "SteeringDisplay.h"

#include "Arial12x12.h"
#include "Arial28x28.h"
#include "font_big.h"
#include "graphics.h"

/* Display Macros */

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
#define BATTERY_TEXT_X 		260
#define VOLTAGE_TEXT_Y 		STATUS_Y + 26
#define SOC_TEXT_Y			STATUS_Y + 6
#define BATTERY_LEFT_X 		150
#define BATTERY_LEFT_Y 		11
#define BATTERY_WIDTH 		100
#define BATTERY_HEIGHT 		30
#define BATTERY_RIGHT_X 	BATTERY_LEFT_X + BATTERY_WIDTH
#define BATTERY_RIGHT_Y 	BATTERY_LEFT_Y + BATTERY_HEIGHT
#define BATTERY_PADDING 	3

#define BATTERY_BTN_X 		250
#define BATTERY_BTN_Y 		20
#define BATTERY_BTN_WIDTH 	2
#define BATTERY_BTN_HEIGHT 	10

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

SteeringDisplay::SteeringDisplay(SPI_TFT_ILI9341* tft) : _tft(tft) { }

void SteeringDisplay::init() {
	// initialize tft display
	_tft->set_orientation(3);
	_tft->background(Black);
	_tft->cls();
	
	// Dms
	_tft->locate(DMS_X, STATUS_Y);
	_tft->printf("DMS");
	_dmsIcon.init(_tft, DMS_X + CIRCLE_X_OFFSET_DMS, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Red, true);
	// Ignition
	_tft->locate(IGNITION_X, STATUS_Y);
	_tft->printf("IGN");
	_ignitionIcon.init(_tft, DMS_X + CIRCLE_X_OFFSET_DMS, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Red, true);
	// Brake
	_tft->locate(BRAKE_X, STATUS_Y);
	_tft->printf("BRK");
	_brakeIcon.init(_tft, DMS_X + CIRCLE_X_OFFSET_DMS, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Red, true);


}