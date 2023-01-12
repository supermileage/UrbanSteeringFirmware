#include "Circle.h"
#include "CircleCollider.h"

Circle::Circle(std::string name, bool enableCollisions) : GameObject(name, enableCollisions) { }

void Circle::init(SPI_TFT_ILI9341* tft, int32_t xpos, int32_t ypos, int32_t colour, int32_t radius, bool fill) {
	Shape::init(tft, xpos, ypos, colour);
	_radius = radius;
	_fill = fill;

	if (_enableCollisions) {
		_collider = new CircleCollider(this, radius);
	}
}

void Circle::draw() {
	_onDraw();
	
	if (_fill)
		_tft->fillcircle(_x, _y, _radius, _colour);
	else
		_tft->circle(_x, _y, _radius, _colour);
}

void Circle::clear() {
	if (_rendered) {
		int x = _lastRenderPosition.x;
		int y = _lastRenderPosition.y;
		_tft->fillcircle(x, y, _radius, _background);
		_rendered = false;
	}
}

void Circle::setRadius(int32_t r) {
	_radius = r;
}

int32_t Circle::getRadius() {
	return _radius;
}