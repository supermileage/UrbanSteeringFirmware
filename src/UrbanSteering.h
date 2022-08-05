#ifndef _URBAN_STEERING_H_
#define _URBAN_STEERING_H_

#include "SteeringDisplay.h"

class UrbanSteering {
	public:
		UrbanSteering(SteeringDisplay* display);
		~UrbanSteering();
		void init();
		void run();

	private:
		SteeringDisplay* _display;
};

#endif