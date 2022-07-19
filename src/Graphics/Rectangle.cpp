#include "Rectangle.h"

Rectangle::Rectangle(SPI_TFT_ILI9341* tft, int32_t xpos, int32_t ypos, int32_t colour, int32_t width, int32_t height, bool fill) :
	Shape(tft, xpos, ypos, colour), _width(width), _height(height), _fill(fill) { }

void Rectangle::draw() {
	if (_fill)
		_tft->fillrect(_x, _y, _x + _width, _y + _height, _colour);
	else
		_tft->rect(_x, _y, _x + _width, _y + _height, _colour);
}	