#ifndef _ANIMATION_FLASHING_H_
#define _ANIMATION_FLASHING_H_

#include "Animation.h"

class AnimationFlashing : public Animation {
	public:
		AnimationFlashing(Shape* shape, int64_t interval) : Animation(shape), _interval(interval) { }
		void run(int64_t millis);
		
	private:
		int64_t _interval;
		bool _isDrawn = false;
};

#endif