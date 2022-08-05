#include "AnimationFlashing.h"

void AnimationFlashing::run(int64_t millis) {
	if (millis > _lastTimeMillis + _interval) {
		_lastTimeMillis = millis;

		if (_isDrawn)
			_shape->clear();
		else
			_shape->draw();

		_isDrawn = !_isDrawn;
	}
}