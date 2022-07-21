#include "Rectangle.h"

void Rectangle::init(SPI_TFT_ILI9341* tft, int32_t xpos1, int32_t ypos1, int32_t colour, int32_t xpos2, int32_t ypos2, bool fill) {
	Shape::init(tft, xpos1, ypos1, colour);
	_x2 = xpos2;
	_y2 = ypos2;
	_fill = fill;
}

void Rectangle::draw() {
	if (_fill)
		_tft->fillrect(_x, _y, _x2, _y2, _colour);
	else
		_tft->rect(_x, _y, _x2, _y2, _colour);
}	