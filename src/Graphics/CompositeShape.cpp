#include "CompositeShape.h"

void CompositeShape::init(const std::vector<Shape*>& components) {
	_components = components;
}

void CompositeShape::draw() {
	for (Shape* shape : _components) {
		shape->draw();
	}
}

void CompositeShape::clear() {
	for (Shape* shape : _components)
		shape->clear();
}

void CompositeShape::addShape(Shape* shape) {
	_components.push_back(shape);
}