#include "Collider.h"

std::vector<Collider*> Collider::_allColliders;

Collider::Collider(GameObject* owner) : _owner(owner) {
    _allColliders.push_back(this);
}

Collider::~Collider() {
    _allColliders.erase(std::remove(_allColliders.begin(), _allColliders.end(), this), _allColliders.end());
}

util::Point Collider::getPosition() {
    return _owner->getPosition();
}

GameObject* Collider::getOwner() {
    return _owner;
}

void Collider::setOwner(GameObject* owner) {
	_owner = owner;
}

Collider::ColliderType Collider::getType() {
    return _type;
}

util::Point Collider::getCenter() {
	return _owner->getPosition();
}

bool Collider::hasCollidedWith(Collider* collider, util::Point& contact) {
    if (_withinCollisionRange(collider)) {
        return _hasCollidedWith(collider, contact);
    }
    return false;
}

bool Collider::_withinCollisionRange(Collider* collider) {
    return util::computeDistance(getCenter(), collider->getCenter()) <=
        getFarthestCollisionDistance() + collider->getFarthestCollisionDistance();
}

/* Static Methods */
const std::vector<Collider*>& Collider::allColliders() {
    return _allColliders;
}