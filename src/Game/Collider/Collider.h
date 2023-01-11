#ifndef _COLLIDER_H_
#define _COLLIDER_H_

#include <vector>

#include "util.h"
#include "GameObject.h"

class GameObject;

class Collider {
    public:
		struct Collision {
            GameObject* obj1;
            GameObject* obj2;
			util::Point contact;

            Collision(GameObject* g1, GameObject* g2, util::Point p) : obj1(g1), obj2(g2), contact(p) { }
        };

        enum ColliderType { Circle, Rectangle };
        Collider(GameObject* owner);
        virtual ~Collider();
        util::Point getPosition();
        GameObject* getOwner();
		void setOwner(GameObject* owner);
        ColliderType getType();
        bool hasCollidedWith(Collider* collider, util::Point& contact);
		virtual util::Point getCenter();
        virtual int32_t getFarthestCollisionDistance() = 0;
        static const std::vector<Collider*>& allColliders();

    protected:
        GameObject* _owner;
        Collider::ColliderType _type;

        bool _withinCollisionRange(Collider* collider);
        virtual bool _hasCollidedWith(Collider* collider, util::Point& contact) = 0;

    private:
        static std::vector<Collider*> _allColliders;
};

#endif