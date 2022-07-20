#ifndef _TEXT_H_
#define _TEXT_H_

#include "Shape.h"

#include <string>

class Text : public Shape {
	public:
		Text() { }
		~Text() { }
		void init(SPI_TFT_ILI9341* tft, int32_t xpos, int32_t ypos, int32_t colour, unsigned char* font) override;
		void draw() override;
		void setDisplayString(std::string value);

	private:
		unsigned char* _font;
		std::string _displayString;
};

#endif