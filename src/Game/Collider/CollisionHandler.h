#ifndef _COLLISION_HANDLER_H_
#define _COLLISION_HANDLER_H_

#include "util.h"
#include "CircleCollider.h"
#include "RectangleCollider.h"

class CircleCollider;
class RectangleCollider;

class CollisionHandler {
    public:
        static CollisionHandler& instance();
        void checkCollisions();
        void registerCollisionDelegate(util::Command* delegate);

        static bool haveCollided(CircleCollider* circle, RectangleCollider* rectangle, util::Point& contact);
        static bool haveCollided(CircleCollider* circle1, CircleCollider* circle2, util::Point& contact);
        static bool haveCollided(RectangleCollider* rectangle1, RectangleCollider* rectangle2, util::Point& contact);

    private:
        static CollisionHandler* _instance;
        util::Command* _collisionDelegate;

        CollisionHandler();
        void _checkCollision(Collider* collider1, Collider* collider2);
};

#endif