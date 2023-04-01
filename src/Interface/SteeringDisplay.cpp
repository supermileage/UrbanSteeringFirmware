#include "SteeringDisplay.h"

#include <stdlib.h>

#include "AnimationFlashing.h"
#include "Arial12x12.h"
#include "font_big.h"
#include "graphics.h"

/* Display Macros */

// fonts
#define SMALL_FONT Arial12x12
#define COOL_FONT Neu42x35

// accessories
#define DMS_X 						100
#define IGNITION_X 					10
#define BRAKE_X						55
#define STATUS_Y 					5
#define CIRCLE_RADIUS 				10
#define CIRCLE_RADIUS_CAN 			5
#define CIRCLE_Y_OFFSET 			CIRCLE_RADIUS * 3
#define CIRCLE_X_OFFSET_DMS  		14
#define CIRCLE_X_OFFSET_IGNITION  	15
#define CIRCLE_X_OFFSET_BRAKE  		14

#define CAN_X						280
#define CAN_X_OFFSET				30
#define CAN_TELEMETRY_Y				180
#define CAN_TELEMETRY_Y_OFFSET		4 //CIRCLE_RADIUS_CAN * 3


// voltage
#define BATTERY_TEXT_X 260
#define VOLTAGE_TEXT_Y STATUS_Y + 26
#define SOC_TEXT_Y STATUS_Y + 6
#define BATTERY_LEFT_X 150
#define BATTERY_LEFT_Y 11
#define BATTERY_WIDTH 100
#define BATTERY_HEIGHT 30
#define BATTERY_RIGHT_X BATTERY_LEFT_X + BATTERY_WIDTH
#define BATTERY_RIGHT_Y BATTERY_LEFT_Y + BATTERY_HEIGHT
#define BATTERY_PADDING 3

#define BATTERY_BTN_X 250
#define BATTERY_BTN_Y 20
#define BATTERY_BTN_WIDTH 2
#define BATTERY_BTN_HEIGHT 10

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
#define POWER_X_UNIT_OFFSET 110
#define POWER_Y 135

// time
#define TIME_X_LABEL 46
#define TIME_Y_LABEL 203
#define MINUTES_X 80
#define COLON_X 160
#define SECONDS_X 185
#define TIME_Y 185

// throttle debug
#define THROTTLE_RAW_X 10
#define THROTTLE_RAW_Y 210

// turn signals
#define TURN_FLASHING_INTERVAL 500
#define TURN_WIDTH 30
#define TURN_HEIGHT 30
#define TURN_LEFT_X 10
#define TURN_LEFT_Y 55
#define TURN_RIGHT_X 280
#define TURN_RIGHT_Y 60

// lights
#define LIGHTS_X 140
#define LIGHTS_Y 55
#define LIGHTS_WIDTH 40
#define LIGHTS_HEIGHT 30

SteeringDisplay::SteeringDisplay(SPI_TFT_ILI9341* tft) : _tft(tft) {
    _animationTimer.start();
}

void SteeringDisplay::init() {
    // initialize tft display
    _tft->set_orientation(3);
    _tft->background(Black);
    _tft->cls();

    /* Small Font Graphics */
    _tft->set_font((unsigned char*)SMALL_FONT);

    // Dms
    _tft->locate(DMS_X, STATUS_Y);
    _tft->printf("DMS");
    _dmsIcon.init(_tft, DMS_X + CIRCLE_X_OFFSET_DMS, CIRCLE_Y_OFFSET, Red, CIRCLE_RADIUS, true);
    _setDynamicGraphic(SteeringDisplay::Dms, &_dmsIcon);

    // Ignition
    _tft->locate(IGNITION_X, STATUS_Y);
    _tft->printf("RUN");
    _ignitionIcon.init(_tft, IGNITION_X + CIRCLE_X_OFFSET_IGNITION, CIRCLE_Y_OFFSET, Red, CIRCLE_RADIUS, true);
    _setDynamicGraphic(SteeringDisplay::Ignition, &_ignitionIcon);

	//CAN telemetry
	_tft->locate(CAN_X, CAN_TELEMETRY_Y);
	_tft->printf("Tel");
	_canTelemetryIcon.init(_tft, CAN_X + CAN_X_OFFSET, CAN_TELEMETRY_Y + CAN_TELEMETRY_Y_OFFSET, Red, CIRCLE_RADIUS_CAN, true);
	_setDynamicGraphic(SteeringDisplay::Telemetry, &_canTelemetryIcon);

	// Battery icon w/ static outline graphic
	_tft->rect(BATTERY_LEFT_X, BATTERY_LEFT_Y, BATTERY_LEFT_X + BATTERY_WIDTH, BATTERY_LEFT_Y + BATTERY_HEIGHT, White);
	_tft->rect(BATTERY_BTN_X, BATTERY_BTN_Y, BATTERY_BTN_X + BATTERY_BTN_WIDTH, BATTERY_BTN_Y + BATTERY_BTN_HEIGHT, White);
	_batteryIcon.init(_tft, BATTERY_LEFT_X + BATTERY_PADDING, BATTERY_LEFT_Y + BATTERY_PADDING,
		Green, BATTERY_RIGHT_X - BATTERY_PADDING, BATTERY_RIGHT_Y - BATTERY_PADDING, true);
	_setDynamicGraphic(SteeringDisplay::Battery, &_batteryIcon);
	// Battery Soc
	_initializeDynamicText(&_batterySocText, SteeringDisplay::Soc, BATTERY_TEXT_X, SOC_TEXT_Y, (unsigned char*)SMALL_FONT, "00.0 %%");
	// Battery Voltage
	_initializeDynamicText(&_batteryVoltageText, SteeringDisplay::Voltage, BATTERY_TEXT_X, VOLTAGE_TEXT_Y, (unsigned char*)SMALL_FONT, "00.0 V");

    // Brake
    _tft->locate(BRAKE_X, STATUS_Y);
    _tft->printf("BRK");
    _brakeIcon.init(_tft, BRAKE_X + CIRCLE_X_OFFSET_BRAKE, CIRCLE_Y_OFFSET, Green, CIRCLE_RADIUS, true);
    _setDynamicGraphic(SteeringDisplay::Brake, &_brakeIcon);

    // Battery icon w/ static outline graphic
    _tft->rect(BATTERY_LEFT_X, BATTERY_LEFT_Y, BATTERY_LEFT_X + BATTERY_WIDTH, BATTERY_LEFT_Y + BATTERY_HEIGHT, White);
    _tft->rect(BATTERY_BTN_X, BATTERY_BTN_Y, BATTERY_BTN_X + BATTERY_BTN_WIDTH, BATTERY_BTN_Y + BATTERY_BTN_HEIGHT, White);
    _batteryIcon.init(_tft, BATTERY_LEFT_X + BATTERY_PADDING, BATTERY_LEFT_Y + BATTERY_PADDING,
                      Green, BATTERY_RIGHT_X - BATTERY_PADDING, BATTERY_RIGHT_Y - BATTERY_PADDING, true);
    _setDynamicGraphic(SteeringDisplay::Battery, &_batteryIcon);
    // Battery Soc
    _initializeDynamicText(&_batterySocText, SteeringDisplay::Soc, BATTERY_TEXT_X, SOC_TEXT_Y, (unsigned char*)SMALL_FONT, "00.0 %%");
    // Battery Voltage
    _initializeDynamicText(&_batteryVoltageText, SteeringDisplay::Voltage, BATTERY_TEXT_X, VOLTAGE_TEXT_Y, (unsigned char*)SMALL_FONT, "00.0 V");

    // Cool Font Graphics Labels
    _tft->locate(SPEED_X_LABEL, SPEED_Y_LABEL);
    _tft->printf("SPD");
    _tft->locate(POWER_X_LABEL, POWER_Y_LABEL);
    _tft->printf("PWR");
    _tft->locate(TIME_X_LABEL, TIME_Y_LABEL);
    _tft->printf("TIME");

    /* Cool Font Graphics */
    _tft->set_font((unsigned char*)COOL_FONT);

    // Speed

    _tft->locate(SPEED_X + SPEED_X_UNIT_OFFSET, SPEED_Y);
    _tft->printf("K/H");
    _initializeDynamicText(&_speedText, SteeringDisplay::Speed, SPEED_X, SPEED_Y, (unsigned char*)COOL_FONT, "00");
    // Throttle
    _tft->locate(POWER_X + POWER_X_UNIT_OFFSET, POWER_Y);
    _tft->printf("%%");
    _initializeDynamicText(&_powerText, SteeringDisplay::Power, POWER_X, POWER_Y, (unsigned char*)COOL_FONT, "000");
    // Time
    _tft->locate(COLON_X, TIME_Y);
    _tft->printf(":");
    _initializeDynamicText(&_timeTextMinutes, SteeringDisplay::Minutes, MINUTES_X, TIME_Y, (unsigned char*)COOL_FONT, "00");
    _initializeDynamicText(&_timeTextSeconds, SteeringDisplay::Seconds, SECONDS_X, TIME_Y, (unsigned char*)COOL_FONT, "00");

    /* Bitmap Graphics */

    // Headlights
    _lights.init(_tft, LIGHTS_X, LIGHTS_Y, LIGHTS_WIDTH, LIGHTS_HEIGHT, graphicLights);
    _dynamicGraphics[SteeringDisplay::Lights] = &_lights;
    // Left Signal
    _leftSignal.init(_tft, TURN_LEFT_X, TURN_LEFT_Y, TURN_WIDTH, TURN_HEIGHT, graphicLeftArrow);
    _dynamicGraphics[SteeringDisplay::LeftSignal] = &_leftSignal;
    // Right Signal
    _rightSignal.init(_tft, TURN_RIGHT_X, TURN_RIGHT_Y, TURN_WIDTH, TURN_HEIGHT, graphicRightArrow);
    _dynamicGraphics[SteeringDisplay::RightSignal] = &_rightSignal;

    _runRedrawQueue();
}

void SteeringDisplay::run() {
    _runRedrawQueue();

    // run action queue
    while (!_actionQueue.empty()) {
        auto& action = _actionQueue.front();
        _actionQueue.pop();
        (action)();
    }

    // run time-based animations
    int64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(_animationTimer.elapsed_time()).count();
    for (const auto& pair : _animations)
        pair.second->run(currentTime);
}

void SteeringDisplay::_runRedrawQueue() {
    while (!_redrawActionQueue.empty()) {
        RedrawAction action = _redrawActionQueue.front();
        _redrawActionQueue.pop();
        (action.shape->*action.method)();
    }
}

Command* SteeringDisplay::_getDelegateForGraphicId(SteeringDisplay::DynamicGraphicId id) {
    switch (id) {
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
        case SteeringDisplay::Lights:
            return new Delegate<SteeringDisplay, data_t>(this, &SteeringDisplay::_onLightsChanged);
            break;
        case SteeringDisplay::LeftSignal:
            return new Delegate<SteeringDisplay, data_t>(this, &SteeringDisplay::_onLeftSignalChanged);
            break;
        case SteeringDisplay::RightSignal:
            return new Delegate<SteeringDisplay, data_t>(this, &SteeringDisplay::_onRightSignalChanged);
            break;
        case SteeringDisplay::Minutes:
            return new Delegate<SteeringDisplay, steering_time_t>(this, &SteeringDisplay::_onTimeChanged);
            break;
        case SteeringDisplay::Hazards:
            return new Delegate<SteeringDisplay, data_t>(this, &SteeringDisplay::_onBlinkChanged);
            break;
        default:
            // do nothing
            break;
    }

    return nullptr;
}

void SteeringDisplay::_setDynamicGraphic(DynamicGraphicId id, Shape* shape) {
    _dynamicGraphics[id] = shape;
    _redrawActionQueue.push(RedrawAction{shape, &Shape::draw});
}

void SteeringDisplay::_initializeDynamicText(Text* textField, SteeringDisplay::DynamicGraphicId id, int32_t xpos, int32_t ypos, unsigned char* font, std::string str) {
    textField->init(_tft, xpos, ypos, font, str);
    _setDynamicGraphic(id, textField);
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
    _batteryIcon.scale(value);
    _redrawActionQueue.push(RedrawAction{&_batteryIcon, &Shape::draw});
}

void SteeringDisplay::_onVoltageChanged(const batt_t value) {
    _updateTextField(SteeringDisplay::Voltage, _batteryDataToString(value));
}

void SteeringDisplay::_onSpeedChanged(const speed_t value) {
    char buf[4] = {};
    sprintf(buf, "%02u", value);
    _updateTextField(SteeringDisplay::Speed, std::string(buf));
}

void SteeringDisplay::_onPowerChanged(const throttle_t value) {
    char buf[4] = {};
    sprintf(buf, "%03u", (value * 100) / 255);
    _updateTextField(SteeringDisplay::Power, std::string(buf));
}

void SteeringDisplay::_onLightsChanged(const data_t value) {
    RedrawAction action{&_lights, 0x0};

    if (value)
        action.method = &Shape::draw;
    else
        action.method = &Shape::clear;

    _redrawActionQueue.push(action);
}

void SteeringDisplay::_onLeftSignalChanged(const data_t value) {
    _handleAnimationChanged(SteeringDisplay::LeftSignal, !value);
}

void SteeringDisplay::_onRightSignalChanged(const data_t value) {
    _handleAnimationChanged(SteeringDisplay::RightSignal, !value);
}

void SteeringDisplay::_onBlinkChanged(const data_t value) {
	RedrawAction action_left { &_leftSignal, 0x0 };
	RedrawAction action_right { &_rightSignal, 0x0 };

	if (value){
		action_left.method = &Shape::draw;
		action_right.method = &Shape::draw;
	}
	else{
		action_left.method = &Shape::clear;
		action_right.method = &Shape::clear;
	}

	_redrawActionQueue.push(action_left);
	_redrawActionQueue.push(action_right);
}

void SteeringDisplay::_onTimeChanged(const steering_time_t value) {
    char buf[4] = {};
    if (_lastTime.minutes != value.minutes) {
        _lastTime.minutes = value.minutes;
        sprintf(buf, "%02d", value.minutes);
        _updateTextField(SteeringDisplay::Minutes, std::string(buf));
    }

    if (_lastTime.seconds != value.seconds) {
        _lastTime.seconds = value.seconds;
        sprintf(buf, "%02d", value.seconds);
        _updateTextField(SteeringDisplay::Seconds, std::string(buf));
    }
}

void SteeringDisplay::_updateCircleIcon(DynamicGraphicId id, data_t value) {
    auto& circle = _dynamicGraphics[id];
    circle->setColour(int32_t(value ? Green : Red));
    _redrawActionQueue.push(RedrawAction{circle, &Shape::draw});
}

void SteeringDisplay::_updateTextField(DynamicGraphicId id, const std::string& value) {
    auto& shape = _dynamicGraphics[id];
    ((Text*)shape)->setDisplayString(value);
    _redrawActionQueue.push(RedrawAction{shape, &Shape::draw});
}

const std::string SteeringDisplay::_batteryDataToString(const batt_t value) {
    char buf[6] = {};
    uint8_t decimal = (uint16_t)(value * 10) % 10;
    sprintf(buf, "%02d.%d", (uint8_t)value, decimal);
    return std::string(buf);
}

void SteeringDisplay::_handleAnimationChanged(DynamicGraphicId id, bool terminate) {
    if (terminate)
        _actionQueue.push([this, id]() -> void {
            if (_animations.find(id) != _animations.end()) {
                Animation* toDelete = _animations[id];
                toDelete->stop();
                _animations.erase(id);
                delete toDelete;
            }
        });
    else
        _actionQueue.push([this, id]() -> void {
            if (_animations.find(id) == _animations.end())
                _animations[id] = new AnimationFlashing(_dynamicGraphics[id], TURN_FLASHING_INTERVAL);
        });
}
