#include "SteeringDisplay.h"

#include <stdlib.h>
#include "Arial12x12.h"
#include "font_big.h"

/* Display Macros */

// fonts 
#define SMALL_FONT 	Arial12x12

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
	
	/* Small Font Graphics */
	_tft->set_font((unsigned char*) SMALL_FONT);
	
	// Dms
	_tft->locate(DMS_X, STATUS_Y);
	_tft->printf("DMS");
	_dmsIcon.init(_tft, DMS_X + CIRCLE_X_OFFSET_DMS, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Red, true);
	_dynamicGraphicsData[(int32_t)SteeringDisplay::Dms] = DynamicGraphicData { &_dmsIcon, true  };
	// Ignition
	_tft->locate(IGNITION_X, STATUS_Y);
	_tft->printf("IGN");
	_ignitionIcon.init(_tft, IGNITION_X + CIRCLE_X_OFFSET_IGNITION, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Red, true);
	_dynamicGraphicsData[(int32_t)SteeringDisplay::Ignition] = DynamicGraphicData { &_ignitionIcon, true };
	// Brake
	_tft->locate(BRAKE_X, STATUS_Y);
	_tft->printf("BRK");
	_brakeIcon.init(_tft, BRAKE_X + CIRCLE_X_OFFSET_BRAKE, CIRCLE_Y_OFFSET, CIRCLE_RADIUS, Green, true);
	_dynamicGraphicsData[(int32_t)SteeringDisplay::Brake] = DynamicGraphicData { &_brakeIcon, true };
	// Battery Icon
	_tft->rect(BATTERY_LEFT_X, BATTERY_LEFT_Y, BATTERY_LEFT_X + BATTERY_WIDTH, BATTERY_LEFT_Y + BATTERY_HEIGHT, White);
	_tft->rect(BATTERY_BTN_X, BATTERY_BTN_Y, BATTERY_BTN_X + BATTERY_BTN_WIDTH, BATTERY_BTN_Y + BATTERY_BTN_HEIGHT, White);
	_batteryIcon.init(_tft, BATTERY_LEFT_X + BATTERY_PADDING, BATTERY_LEFT_Y + BATTERY_PADDING, Green, BATTERY_RIGHT_X - BATTERY_PADDING, BATTERY_RIGHT_Y - BATTERY_PADDING, true);
	_dynamicGraphicsData[(int32_t)SteeringDisplay::Battery] = DynamicGraphicData { &_batteryIcon, true };

	// Battery Soc
	_initializeDynamicText(&_batterySocText, SteeringDisplay::Soc, BATTERY_TEXT_X, SOC_TEXT_Y, (unsigned char*)SMALL_FONT, "00.0 %");
	// Battery Voltage
	_initializeDynamicText(&_batteryVoltageText, SteeringDisplay::Voltage, BATTERY_TEXT_X, VOLTAGE_TEXT_Y, (unsigned char*)SMALL_FONT, "00.0 V");

	/* Cool Font Graphics */
	_tft->set_font((unsigned char*) COOL_FONT);

	// Speed
	_tft->locate(SPEED_X_LABEL, SPEED_Y_LABEL);
	_tft->printf("SPD");
	_tft->locate(SPEED_X + SPEED_X_UNIT_OFFSET, SPEED_Y);
	_tft->printf("K/H");
	_initializeDynamicText(&_speedText, SteeringDisplay::Speed, SPEED_X, SPEED_Y, (unsigned char*)COOL_FONT, "00");
	// Throttle
	_tft->locate(POWER_X_LABEL, POWER_Y_LABEL);
	_tft->printf("PWR");
	_tft->locate(POWER_X + POWER_X_UNIT_OFFSET, POWER_Y);
	_tft->printf("%");
	_initializeDynamicText(&_powerText, SteeringDisplay::Power, POWER_X, POWER_Y, (unsigned char*)COOL_FONT, "000");
	// Time
	_tft->locate(TIME_X_LABEL, TIME_Y_LABEL);
	_tft->printf("TIME");
	_tft->locate(COLON_X, TIME_Y);
	_tft->printf(":");
	_initializeDynamicText(&_timeTextMinutes, SteeringDisplay::Minutes, MINUTES_X, TIME_Y, (unsigned char*)COOL_FONT, "00");
	_initializeDynamicText(&_timeTextSeconds, SteeringDisplay::Seconds, SECONDS_X, TIME_Y, (unsigned char*)COOL_FONT, "00");

	#if DEBUG_THROTTLE
		TFT.locate(THROTTLE_RAW_X, THROTTLE_RAW_Y);
		TFT.printf("0000");
	#endif

	_runDynamicGraphics();
}

void SteeringDisplay::run() {
	_runDynamicGraphics();
}

Command* SteeringDisplay::_getCommandForGraphic(SteeringDisplay::DynamicGraphic graphic) {
	switch (graphic) {
		case SteeringDisplay::Dms:
			return new Delegate<SteeringDisplay, data_t>(this, &SteeringDisplay::_onDmsChanged);
			break;
		case SteeringDisplay::Ignition:
			return new Delegate<SteeringDisplay, data_t>(this, &SteeringDisplay::_onIgnitionChanged);
			break;
		case SteeringDisplay::Brake:
			return new Delegate<SteeringDisplay, data_t>(this, &SteeringDisplay::_onBrakeChanged);
			break;
		case SteeringDisplay::Battery:
			// Battery icon is updated with new soc / voltage data
			break;
		case SteeringDisplay::Soc:
			return new Delegate<SteeringDisplay, batt_t>(this, &SteeringDisplay::_onBatterySocChanged);
			break;
		case SteeringDisplay::Voltage:
			return new Delegate<SteeringDisplay, batt_t>(this, &SteeringDisplay::_onVoltageChanged);
			break;
		case SteeringDisplay::Speed:
			return new Delegate<SteeringDisplay, speed_t>(this, &SteeringDisplay::_onSpeedChanged);
			break;
		case SteeringDisplay::Power:
			return new Delegate<SteeringDisplay, throttle_t>(this, &SteeringDisplay::_onPowerChanged);
			break;
		default:
			// do nothing
			break;
	}
}

void SteeringDisplay::_runDynamicGraphics() {
	for (int32_t i = SteeringDisplay::Dms; i <= SteeringDisplay::Seconds; i++) {
		DynamicGraphicData data = _dynamicGraphicsData[i];
		
		if (data.redraw)
			data.shape->draw();
	}
		
}

void SteeringDisplay::_initializeDynamicText(Text* textField, SteeringDisplay::DynamicGraphic graphic, int32_t xpos, int32_t ypos, unsigned char* font, std::string str) {
	_powerText.init(_tft, xpos, ypos, font);
	_powerText.setDisplayString(str);
	_dynamicGraphicsData[(int32_t)graphic] = DynamicGraphicData { textField, true };
}

void SteeringDisplay::_onDmsChanged(const data_t value) {
	_updateCircleIcon(SteeringDisplay::Dms, value);
}

void SteeringDisplay::_onIgnitionChanged(const data_t value) {
	_updateCircleIcon(SteeringDisplay::Ignition, value);
}

void SteeringDisplay::_onBrakeChanged(const data_t value) {
	_updateCircleIcon(SteeringDisplay::Brake, !value);
}

void SteeringDisplay::_onBatterySocChanged(const batt_t value) {
	_updateTextField(SteeringDisplay::Soc, _batteryDataToString(value));
}

void SteeringDisplay::_onVoltageChanged(const batt_t value) {
	_updateTextField(SteeringDisplay::Voltage, _batteryDataToString(value));
}

void SteeringDisplay::_onSpeedChanged(const speed_t value) {
	char buf[4] = { };
	sprintf(buf, "%02u", value);
	_updateTextField(SteeringDisplay::Speed, std::string(buf));
}

void SteeringDisplay::_onPowerChanged(const throttle_t value) {
	char buf[4] = { };
	sprintf(buf, "%03u", (value * 100) / 255);
}

void SteeringDisplay::_updateCircleIcon(DynamicGraphic graphic, data_t value) {
	auto& data = _dynamicGraphicsData[(int32_t)graphic];
	data.shape->setColour(int32_t(value & 0x1 ? Green : Red));
	data.redraw = true;
}

void SteeringDisplay::_updateTextField(DynamicGraphic graphic, const std::string& value) {
	auto& data = _dynamicGraphicsData[(int32_t)graphic];
	((Text*)data.shape)->setDisplayString(value);
	data.redraw = true;
}

const std::string _batteryDataToString(const batt_t value) {
	char buf[6] = { };
	uint8_t decimal = (uint16_t)(value * 10) % 10;
	sprintf(buf, "%02d.%d %", (uint8_t)value, decimal);
	return std::string(buf);
}
