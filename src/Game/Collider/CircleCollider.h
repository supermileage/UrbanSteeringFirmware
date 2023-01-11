#ifndef _CIRCLE_COLLIDER_H_
#define _CIRCLE_COLLIDER_H_

#include "Collider.h"
#include "CollisionHandler.h"
#include "RectangleCollider.h"

class CollisionHandler;
class RectangleCollider;

class CircleCollider : public Collider {
    public:
        CircleCollider(GameObject* owner, int32_t radius);
        void setRadius(int32_t r);
        int32_t getRadius();
        int32_t getFarthestCollisionDistance() override;


    private:
        int32_t _radius;

        bool _hasCollidedWith(Collider* collider, util::Point& contact) override;
};

#endif