#ifndef _STEERING_DISPLAY_H_
#define _STEERING_DISPLAY_H_

#include <mstd_functional>
#include <unordered_map>
#include <mbed.h>

#include "Mutex.h"
#include "SPI_TFT_ILI9341.h"
#include "util.h"
#include "data-types.h"

#include "SharedProperty.h"
#include "ThreadedQueue.h"
#include "Circle.h"
#include "Rectangle.h"
#include "ScalableRectangle.h"
#include "Text.h"
#include "Bitmap.h"
#include "CompositeShape.h"
#include "Animation.h"

using namespace util;

#define DEBUG_THROTTLE 0

class SteeringDisplay {
	public:
		enum DynamicGraphicId { Dms, Ignition, Brake, Battery, Soc, Voltage, Speed, Power, Lights, LeftSignal, RightSignal, Minutes, Seconds, Hazards, Telemetry, Bms, Throttle};
		SteeringDisplay(SPI_TFT_ILI9341* tft);
		~SteeringDisplay() { }
		void init();
		void run();

		template <class T>
		void addDynamicGraphicBinding(SharedProperty<T>& property, DynamicGraphicId id) {
			Command* command = _getDelegateForGraphicId(id);
			property.addValueChangedListener(command);
		}

	private:
		struct RedrawAction {
			Shape* shape;
			void (Shape::*method)(void);
		};

		struct InternalAction {
			void (SteeringDisplay::*method)(data_t);
			data_t data;
		};

		SPI_TFT_ILI9341* _tft;
		std::unordered_map<DynamicGraphicId, Shape*> _dynamicGraphics;	// id (as index) to dynamic graphics map
		std::unordered_map<DynamicGraphicId, Animation*> _animations;	// graphic id to timed animation map
		ThreadedQueue<RedrawAction> _redrawActionQueue;					// queue of actions: main thread adds to this, ui thread executes
		ThreadedQueue<std::function<void(void)>> _actionQueue;			// queue of generic functions (can include lambdas with captures)
		Timer _animationTimer;											// timer for animations to keep track of their states
		steering_time_t _lastTime;
		// Dynamic Graphics (these are bound to external shared properties)
		Circle _dmsIcon;
		Circle _ignitionIcon;
		Circle _brakeIcon;
		Circle _canTelemetryIcon;
		Circle _canAccessoriesIcon;
		Circle _canBmsIcon;
		Circle _canThrottleIcon;
		ScalableRectangle _batteryIcon;
		Text _batterySocText;
		Text _batteryVoltageText;
		Text _speedText;
		Text _powerText;
		Text _timeTextMinutes;
		Text _timeTextSeconds;
		Bitmap _lights;
		Bitmap _leftSignal;
		Bitmap _rightSignal;
		
		void _runRedrawQueue();

		// Initialization helpers
		Command* _getDelegateForGraphicId(DynamicGraphicId id);
		void _setDynamicGraphic(DynamicGraphicId id, Shape* shape);
		void _initializeDynamicText(Text* textField, DynamicGraphicId id, int32_t xpos, int32_t ypos, unsigned char* font, std::string str);

		// Data changed event callbacks (these are latched to relevant property changed events)
		void _onDmsChanged(const data_t value);
		void _onIgnitionChanged(const data_t value);
		void _onBrakeChanged(const data_t value);
		void _onCanTelNotDetected(const data_t value);
		void _onCanAccNotDetected(const data_t value);
		void _onCanBmsNotDetected(const data_t value);
		void _onCanThrottleNotDetected(const data_t value);
		void _onBatterySocChanged(const batt_t value);
		void _onVoltageChanged(const batt_t value);
		void _onSpeedChanged(const speed_t value);
		void _onPowerChanged(const throttle_t value);
		void _onLightsChanged(const data_t value);
		void _onLeftSignalChanged(const data_t value);
		void _onRightSignalChanged(const data_t value);
		void _onTimeChanged(const steering_time_t value);
		void _onBlinkChanged(const data_t value);

		// Data changed helpers
		void _updateCircleIcon(DynamicGraphicId id, data_t value);
		void _updateCanCircleIcon(DynamicGraphicId id, data_t value);
		void _updateTextField(DynamicGraphicId id, const std::string& value);
		const std::string _batteryDataToString(const batt_t value);
		void _handleAnimationChanged(DynamicGraphicId id, bool value);

};

#endif