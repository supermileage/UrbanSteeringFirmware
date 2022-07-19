#ifndef _CIRCLE_H_
#define _CIRCLE_H_

#include "Shape.h"

class Circle : public Shape {
	public:
		Circle(SPI_TFT_ILI9341* tft, int32_t xpos, int32_t ypos, int32_t colour, int32_t radius, bool _fill);
		~Circle() { }
		void draw() override;
		
	private:
		int32_t _radius;
		bool _fill;
};

#endif