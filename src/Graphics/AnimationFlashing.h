#ifndef _ANIMATION_FLASHING_H_
#define _ANIMATION_FLASHING_H_

#include "Animation.h"

class AnimationFlashing : public Animation {
	public:
		AnimationFlashing(Shape* shape, int32_t interval) : Animation(shape), _interval(interval) { }
		void run(int32_t millis);
		
	private:
		int32_t _interval;
		bool _isDrawn = false;
};

#endif