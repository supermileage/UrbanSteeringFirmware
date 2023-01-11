#ifndef _INPUT_MANAGER_H_
#define _INPUT_MANAGER_H_

#include "GameBase.h"
#include "util.h"

class GameBase;

using namespace util;

// TODO: implement this for steering
class InputManager {
    public:
        InputManager() { }
        ~InputManager() { }
        void init() { }
        void run() { }
        void setGame(GameBase* game) { }
        void setButtonPressedCallback1(void (GameBase::*callback)(void)) { }
        void setButtonReleasedCallback1(void (GameBase::*callback)(void)) { }
        void setButtonPressedCallback2(void (GameBase::*callback)(void)) { }
        void setButtonReleasedCallback2(void (GameBase::*callback)(void)) { }
        void setJoystickChangedCallback(void (GameBase::*callback)(Vec2)) { }
        void setCloseRequestCallback(void (GameBase::*callback)(void)) { }

        /* Returns 'joystick' position with floating point values [-1,1] on x, y */
        Vec2 currentJoystickPos() {
            return Vec2 { 0, 0}; // stub
        }

    private:

};

#endif