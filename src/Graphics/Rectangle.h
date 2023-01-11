#ifndef _RECTANGLE_H_
#define _RECTANGLE_H_

#include "GameObject.h"

class Rectangle : public GameObject {
	public:
		Rectangle(std::string name = "", bool enableCollisions = false);
		~Rectangle() { }
		void init(SPI_TFT_ILI9341* tft, int32_t x1, int32_t y1, int32_t colour, int32_t x2, int32_t y2, bool fill);
		void init(SPI_TFT_ILI9341* tft, util::Point topLeft, int32_t width, int32_t height, int32_t colour, bool fill);
		void draw() override;
		void clear() override;
		bool move() override;
		int32_t getWidth();
		int32_t getHeight();

	protected:
		int32_t _x2;
		int32_t _y2;
		int32_t _width;
		int32_t _height;
		bool _fill;
};

#endif