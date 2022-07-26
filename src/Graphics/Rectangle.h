#ifndef _RECTANGLE_H_
#define _RECTANGLE_H_

#include "Shape.h"

class Rectangle : public Shape {
	public:
		Rectangle() { }
		~Rectangle() { }
		void init(SPI_TFT_ILI9341* tft, int32_t xpos, int32_t ypos, int32_t colour, int32_t xpos2, int32_t ypos2, bool fill);
		void draw() override;
		void clear() override;

	protected:
		int32_t _x2;
		int32_t _y2;
		bool _fill;
};

#endif