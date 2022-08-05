#include "Text.h"

void Text::init(SPI_TFT_ILI9341* tft, int32_t xpos, int32_t ypos, unsigned char* font, const std::string& defaultString) {
	Shape::init(tft, xpos, ypos, White);
	_font = font;
	_displayString = defaultString;
	_defaultString = defaultString;
}

void Text::draw() {
	_tft->set_font(_font);
	_tft->locate(_x, _y);
	_tft->printf(_displayString.c_str());
}

void Text::setDisplayString(const std::string& value) {
	_displayString = value;
}

void Text::clear() {
	_tft->set_font(_font);
	_tft->locate(_x, _y);
	_tft->printf(_defaultString.c_str());
}