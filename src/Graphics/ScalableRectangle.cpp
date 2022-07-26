#include "ScalableRectangle.h"

void ScalableRectangle::init(SPI_TFT_ILI9341* tft, int32_t xpos, int32_t ypos, int32_t colour, int32_t xpos2, int32_t ypos2, bool fill) {
	Rectangle::init(tft, xpos, ypos, colour, xpos2, ypos2, fill);
	_totalWidth = xpos2 - xpos;
	_scaledWidth = _totalWidth;
}

void ScalableRectangle::draw() {
	_tft->fillrect(_x, _y, _x + _scaledWidth, _y2, _colour);

	if (_scaledWidth != _totalWidth)
		_tft->fillrect(_x + _scaledWidth, _y, _x2, _y2, _background);
}

void ScalableRectangle::scale(float percentage) {
	_scaledWidth = (int32_t)(percentage / 100.0f * (float)_totalWidth);
}