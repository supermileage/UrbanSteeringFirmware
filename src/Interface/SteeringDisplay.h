#ifndef _STEERING_DISPLAY_H_
#define _STEERING_DISPLAY_H_

#include <vector>
#include "mbed.h"
#include "Mutex.h"
#include "SPI_TFT_ILI9341.h"
#include "util.h"
#include "data-types.h"

#include "SharedProperty.h"
#include "Circle.h"
#include "Rectangle.h"
#include "Text.h"

using namespace util;

#define DEBUG_THROTTLE 0

class SteeringDisplay {
	public:
		enum DynamicGraphic { Dms, Ignition, Brake, Battery, Soc, Voltage, Speed, Power, Minutes, Seconds };
		SteeringDisplay(SPI_TFT_ILI9341* tft);
		~SteeringDisplay() { }
		void init();
		void run();

		template <class T>
		void addDynamicGraphicBinding(SharedProperty<T>& property, DynamicGraphic graphic) {
			Command* command = _getCommandForGraphic(graphic);
			property.addValueChangedListener(command);
		}

	private:
		struct DynamicGraphicData {
			Shape* shape;
			bool redraw = false;
		};

		SPI_TFT_ILI9341* _tft;
		std::vector<DynamicGraphicData> _dynamicGraphicsData;
		Timer _timer;
		int32_t _lastTimeMinutes;
		int32_t _lastTimeSeconds;
		// Graphics
		Circle _dmsIcon;
		Circle _ignitionIcon;
		Circle _brakeIcon;
		Rectangle _batteryIcon;
		Text _batterySocText;
		Text _batteryVoltageText;
		Text _speedText;
		Text _powerText;
		Text _timeTextMinutes;
		Text _timeTextSeconds;

		Command* _getCommandForGraphic(DynamicGraphic graphic);
		void _runDynamicGraphics();
		void _initializeDynamicText(Text* textField, DynamicGraphic graphic, int32_t xpos, int32_t ypos, unsigned char* font, std::string str);
		void _updateCircleIcon(DynamicGraphic graphic, data_t value);
		void _updateTextField(DynamicGraphic graphic, const std::string& value);
		const std::string _batteryDataToString(const batt_t value);
		void _onDmsChanged(const data_t value);
		void _onIgnitionChanged(const data_t value);
		void _onBrakeChanged(const data_t value);
		void _onBatterySocChanged(const batt_t value);
		void _onVoltageChanged(const batt_t value);
		void _onSpeedChanged(const speed_t value);
		void _onPowerChanged(const throttle_t value);

};

#endif