#ifndef _SHAPE_H_
#define _SHAPE_H_

#include "SPI_TFT_ILI9341.h"

class Shape {
	public:
		Shape(SPI_TFT_ILI9341* tft, int32_t xpos, int32_t ypos, int32_t colour);
		virtual ~Shape() { }
		void setColour(int32_t value);
		int32_t getColour();
		virtual void draw() = 0;
		
	protected:
		SPI_TFT_ILI9341* _tft;
		int32_t _x;
		int32_t _y;
		int32_t _colour;
};

#endif