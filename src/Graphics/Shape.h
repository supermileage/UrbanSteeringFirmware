#ifndef _SHAPE_H_
#define _SHAPE_H_

#include "SPI_TFT_ILI9341.h"

class Shape {
	public:
		virtual ~Shape() { }
		void init(SPI_TFT_ILI9341* tft, int32_t xpos, int32_t ypos, int32_t colour, int32_t background = BlackTFT);
		void setColour(int32_t value);
		int32_t getColour();
		virtual void draw() = 0;
		virtual void clear() = 0;
		
	protected:
		SPI_TFT_ILI9341* _tft;
		int32_t _x;
		int32_t _y;
		int32_t _colour;
		int32_t _background;
};

#endif