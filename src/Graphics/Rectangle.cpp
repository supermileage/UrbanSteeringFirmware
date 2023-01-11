#include "Rectangle.h"
#include "RectangleCollider.h"

Rectangle::Rectangle(std::string name, bool enableCollisions) : GameObject(name, enableCollisions) { }

void Rectangle::init(SPI_TFT_ILI9341* tft, int32_t x1, int32_t y1, int32_t colour, int32_t x2, int32_t y2, bool fill) {
	Shape::init(tft, x1, y1, colour);
	_x2 = x2;
	_y2 = y2;
	_width = x2 - x1;
	_height = y2 - y1;
	_fill = fill;

	if (_enableCollisions) {
		_collider = new RectangleCollider(this, x2 - x1, y2 - y1);
	}
}

void Rectangle::init(SPI_TFT_ILI9341* tft, util::Point topLeft, int32_t width, int32_t height, int32_t colour, bool fill) {
	init(tft, topLeft.x, topLeft.y, colour, topLeft.x + width, topLeft.y + height, fill);
}

void Rectangle::draw() {
	if (_fill)
		_tft->fillrect(_x, _y, _x2, _y2, _colour);
	else
		_tft->rect(_x, _y, _x2, _y2, _colour);
}

void Rectangle::clear() {
	_tft->fillrect(_x, _y, _x2, _y2, _background);
}

bool Rectangle::move() {
	float deltaX = _speed * _direction.x + _lastDeltaX;
    float deltaY = _speed * _direction.y + _lastDeltaY;
    float roundedX = round(deltaX);
    float roundedY = round(deltaY);

    _lastDeltaX = (roundedX == 0.0f ? deltaX : 0);
    _lastDeltaY = (roundedY == 0.0f ? deltaY : 0);
    _x += roundedX;
    _y += roundedY;
	_x2 += roundedX;
    _y2 += roundedY;
    
    if (roundedX != 0 || roundedY != 0) {
        return true;
    }
    return false;
}

int32_t Rectangle::getWidth() {
	return _width;
}

int32_t Rectangle::getHeight() {
	return _height;
}
