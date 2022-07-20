#ifndef _STEERING_DISPLAY_H_
#define _STEERING_DISPLAY_H_

#include "display-settings.h"
#include "Circle.h"
#include "Text.h"

class SteeringDisplay {
	public:
		SteeringDisplay(SPI_TFT_ILI9341* tft);
		~SteeringDisplay();
		void init();
		void updateTurnSignals(char signal);
		void updateCurrentSpeed();
		
	private:
		SPI_TFT_ILI9341* _tft;
		Circle _dmsIcon;
		Circle _ignitionIcon;
		Circle _brakeIcon;

};

#endif