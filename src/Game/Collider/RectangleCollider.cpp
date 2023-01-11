#include "RectangleCollider.h"

RectangleCollider::RectangleCollider(GameObject* owner, int32_t width, int32_t height) :
 Collider(owner), _width(width), _height(height) {
    _type = Rectangle;
    _farthestDistanceFromCenter = util::computeDistance(util::Point { 0 , 0 }, util::Point { _width, _height });
}

void RectangleCollider::setWidth(int32_t width) {
    _width = width;
}

int32_t RectangleCollider::getWidth() {
    return _width;
}

void RectangleCollider::setHeight(int32_t height) {
    _height = height;
}

int32_t RectangleCollider::getHeight() {
    return _height;
}

util::Point RectangleCollider::getCenter() {
	return util::Point{ _owner->getPosition().x + _width / 2, _owner->getPosition().y + _height / 2 };
}

int32_t RectangleCollider::getFarthestCollisionDistance() {
    return _farthestDistanceFromCenter;
}

bool RectangleCollider::_hasCollidedWith(Collider* collider, util::Point& contact) {
    switch (collider->getType()) {
        case Circle:
            return CollisionHandler::haveCollided((CircleCollider*)collider, this, contact);
            break;
        case Rectangle:
            return CollisionHandler::haveCollided(this, (RectangleCollider*)collider, contact);
            break;
        default:
            return false;
            break;
    }
}