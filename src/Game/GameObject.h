#ifndef _STEERING_GRAPHIC_H_
#define _STEERING_GRAPHIC_H_

#include <vector>
#include <string>

#include "Shape.h"
#include "Collider.h"
#include "util.h"

class Collider;

using namespace util;

// PORTING: it needs to be decided whether or not there should be a separate hierarchy
//          for vanilla UI elements / GameObjects

// represents a game object with position, collider and rendering options
class GameObject : public Shape {
    public:
        GameObject(std::string name, bool enableCollision);
        virtual ~GameObject();
        const std::string& getName();
        void setDirection(Vec2 dir);
		Vec2 getDirection();
		void setSpeed(int32_t speed);
		int32_t getSpeed();
        void setPosition(Point pos);
        Point getPosition();
        void setCollider(Collider* collider);
        Collider* getCollider();
        /* Returns true if position has been updated */
		virtual bool move();

    protected:
        Vec2 _direction = { 0, 0 };
        std::string _name;
		int32_t _speed = 0;
        float _lastDeltaX = 0;
        float _lastDeltaY = 0;
        Collider* _collider = nullptr;
        bool _enableCollisions;
};

#endif