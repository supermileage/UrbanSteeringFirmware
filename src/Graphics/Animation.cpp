#include "Animation.h"

Animation::Animation(Shape* shape) : _shape(shape) {
	_lastTimeMillis = 0;
}

void Animation::stop() {
	_shape->clear();
}