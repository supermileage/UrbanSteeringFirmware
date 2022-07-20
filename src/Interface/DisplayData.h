#ifndef _DISPLAY_DATA_H_
#define _DISPLAY_DATA_H_

#include <stdint.h>

class DisplayData {
	public:
		DisplayData() { }
		~DisplayData() { }
	private:
		unsigned _currentSpeed = 0;
		float _currentBmsSoc = 0.0f;
		float _currentBmsVoltage = 0.0f;
		uint8_t _currentThrottle = 0;
		char _prevAccVal = 0;
		char _initionVal = 0;
		char _dmsVal = 0;
		char _brakeVal = 0;
		char _turnLeft = 0;
		char _turnRight = 0;
		char _lights = 0;
};

#endif