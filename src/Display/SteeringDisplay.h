#ifndef _STEERING_DISPLAY_H_
#define _STEERING_DISPLAY_H_

#include "Shape.h"

class BatteryIcon {
	public:
		BatteryIcon(SPI_TFT_ILI9341* tft, int32_t xpos, int32_t ypos, int32_t width);
		~BatteryIcon();
		void init();
		void updateTurnSignals();
	private:

};

#endif