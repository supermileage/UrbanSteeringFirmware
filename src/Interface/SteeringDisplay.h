#ifndef _STEERING_DISPLAY_H_
#define _STEERING_DISPLAY_H_

#include <vector>

#include "mbed.h"
#include "Mutex.h"
#include "SPI_TFT_ILI9341.h"
#include "util.h"
#include "data-types.h"

#include "SharedProperty.h"
#include "ThreadedQueue.h"
#include "ThreadedMap.h"
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
		enum DynamicGraphicId { Dms, Ignition, Brake, Battery, Soc, Voltage, Speed, Power, Lights, LeftSignal, RightSignal, Minutes, Seconds };
		SteeringDisplay(SPI_TFT_ILI9341* tft);
		~SteeringDisplay() { }
		void init();
		void run();

		template <class T>
		void addDynamicGraphicBinding(SharedProperty<T>& property, DynamicGraphicId id) {
			Command* command = _getDelegateForGraphic(graphic);
			property.addValueChangedListener(command);
		}

	private:
		struct RedrawAction { 
			Shape* shape;
			void (Shape::*method)(void);
		};

		SPI_TFT_ILI9341* _tft;
		std::vector<Shape*> _dynamicGraphics;							// graphics mapped to their respective id
		ThreadedQueue<RedrawAction> _redrawActionQueue;					// queue of individual redraw actions
		ThreadedMap<DynamicGraphicId, Animation*> _animations;			// graphic id to timed animation map
		Timer _animationTimer;											// timer for animations to keep track of their states
		int32_t _lastTimeMinutes;
		int32_t _lastTimeSeconds;
		// Graphics
		Circle _dmsIcon;
		Circle _ignitionIcon;
		Circle _brakeIcon;
		ScalableRectangle _batteryIcon;
		Text _batterySocText;
		Text _batteryVoltageText;
		Text _speedText;
		Text _powerText;
		Text _timeTextMinutes;
		Text _timeTextSeconds;
		Bitmap _lights;
		CompositeShape _leftSignal;
		CompositeShape _rightSignal;

		Command* _getDelegateForGraphic(DynamicGraphicId id);
		void _runRedrawQueue();
		void _setDynamicGraphic(DynamicGraphicId id, Shape* shape);
		void _initializeDynamicText(Text* textField, DynamicGraphicId id, int32_t xpos, int32_t ypos, unsigned char* font, std::string str);
		void _updateCircleIcon(DynamicGraphicId id, data_t value);
		void _updateTextField(DynamicGraphicId id, const std::string& value);
		void _updateTurnSignalAnimation(DynamicGraphicId id, data_t value);
		const std::string _batteryDataToString(const batt_t value);
		
		void _onDmsChanged(const data_t value);
		void _onIgnitionChanged(const data_t value);
		void _onBrakeChanged(const data_t value);
		void _onBatterySocChanged(const batt_t value);
		void _onVoltageChanged(const batt_t value);
		void _onSpeedChanged(const speed_t value);
		void _onPowerChanged(const throttle_t value);
		void _onLightsChanged(const data_t value);
		void _onLeftSignalChanged(const data_t value);
		void _onRightSignalChanged(const data_t value);

};

#endif