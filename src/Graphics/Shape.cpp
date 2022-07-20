#include "Shape.h"

void Shape::init(SPI_TFT_ILI9341* tft, int32_t xpos, int32_t ypos, int32_t colour) {
	_tft = tft;
	_x = xpos;
	_y = ypos;
	_colour = colour;
}

void Shape::setColour(int32_t value) {
	_colour = value;
}

int32_t Shape::getColour() {
	return _colour;
}