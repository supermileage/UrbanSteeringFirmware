#ifndef _ANIMATION_H_
#define _ANIMATION_H_

#include "Shape.h"

class Animation {
	public:
		Animation(Shape* shape) : _shape(shape) {
			_lastTimeMillis = 0;
		}

		~Animation() { }
		virtual void run(int32_t millis) = 0;

	protected:
		Shape* _shape;
		int32_t _lastTimeMillis;
};

#endif