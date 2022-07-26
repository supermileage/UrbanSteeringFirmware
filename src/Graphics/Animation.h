#ifndef _ANIMATION_H_
#define _ANIMATION_H_

#include "Shape.h"

class Animation {
	public:
		Animation(Shape* shape);
		virtual ~Animation() { }
		virtual void run(int64_t millis) = 0;
		virtual void stop();

	protected:
		Shape* _shape;
		int64_t _lastTimeMillis;
};

#endif