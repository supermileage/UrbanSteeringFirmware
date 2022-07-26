#ifndef _COMPOSITE_H_
#define _COMPOSITE_H_

#include <vector>

#include "Shape.h"

class CompositeShape : public Shape {
	public:
		CompositeShape() { }
		~CompositeShape() { }
		void init();
		void init(const std::vector<Shape*>& components);
		void draw() override;
		void clear() override;
		void addShape(Shape* shape);

	private:
		std::vector<Shape*> _components;
};

#endif