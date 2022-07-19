#include "Circle.h"

Circle::Circle(SPI_TFT_ILI9341* tft, int32_t xpos, int32_t ypos, int32_t colour, int32_t radius, bool fill) :
	Shape(tft, xpos, ypos, colour), _radius(radius), _fill(fill) { }

void Circle::draw() {
	if (_fill)
		_tft->fillcircle(_x, _y, _radius, _colour);
	else
		_tft->circle(_x, _y, _radius, _colour);
}