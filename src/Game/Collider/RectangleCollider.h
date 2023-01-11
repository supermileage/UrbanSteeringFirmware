#ifndef _RECTANGLE_COLLIDER_H_
#define _RECTANGLE_COLLIDER_H_

#include "Collider.h"
#include "CollisionHandler.h"
#include "CircleCollider.h"

class CollisionHandler;
class CircleCollider;

class RectangleCollider : public Collider {
    public:
        RectangleCollider(GameObject* owner, int32_t width, int32_t height);
        int32_t getFarthestCollisionDistance() override;
        void setWidth(int32_t width);
        int32_t getWidth();
        void setHeight(int32_t height);
        int32_t getHeight();
		util::Point getCenter() override;
        
    private:
        int32_t _width;
        int32_t _height;
        int32_t _farthestDistanceFromCenter;

        bool _hasCollidedWith(Collider* collider, util::Point& contact) override;
};

#endif