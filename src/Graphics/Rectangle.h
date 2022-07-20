#ifndef _RECTANGLE_H_
#define _RECTANGLE_H_

#include "Shape.h"

class Rectangle : public Shape {
	public:
		Rectangle() { }
		~Rectangle() { }
		void init(SPI_TFT_ILI9341* tft, int32_t xpos, int32_t ypos, int32_t colour, int32_t width, int32_t height, bool fill) override;
		void draw() override;

	private:
		int32_t _width;
		int32_t _height;
		bool _fill;
};

#endif