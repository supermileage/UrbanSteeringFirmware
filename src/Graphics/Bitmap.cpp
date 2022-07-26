#include "Bitmap.h"

void Bitmap::init(SPI_TFT_ILI9341* tft, int32_t xpos, int32_t ypos, int32_t width, int32_t height, unsigned char* bitmap) {
	Shape::init(tft, xpos, ypos, 0);
	_width = width;
	_height = height;
	_bitmap = bitmap;
}

void Bitmap::draw() {
	_tft->Bitmap(_x, _y, _width, _height, _bitmap);
}

void Bitmap::clear() {
	_tft->fillrect(_x, _y, _x + _width, _y + _height, _background);
}