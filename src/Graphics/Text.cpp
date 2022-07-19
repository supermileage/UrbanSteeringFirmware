#include "Text.h"

Text::Text(SPI_TFT_ILI9341* tft, int32_t xpos, int32_t ypos, int32_t colour, unsigned char* font) :
	Shape(tft, xpos, ypos, colour), _font(font) { }

void Text::draw() {
	_tft->set_font(_font);
	_tft->locate(_x, _y);
	_tft->printf(_displayString.c_str());
}