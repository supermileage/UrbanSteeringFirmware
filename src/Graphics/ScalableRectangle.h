#ifndef _SCALED_RECTANGLE_H_
#define _SCALED_RECTANGLE_H_

#include "Rectangle.h"

class ScalableRectangle : public Rectangle {
	public:
		~ScalableRectangle() { }
		void init(SPI_TFT_ILI9341* tft, int32_t xpos, int32_t ypos, int32_t colour, int32_t xpos2, int32_t ypos2, bool fill);
		void draw() override;
		void scale(float percentage);
	
	private:
		int32_t _scaledWidth;
		int32_t _totalWidth;
		
};

#endif