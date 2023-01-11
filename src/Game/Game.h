#ifndef _GAME_H_
#define _GAME_H_

#include "GameBase.h"

class Game : public GameBase {
    public:
        Game(SPI_TFT_ILI9341* tft, InputManager* inputManager) : GameBase(tft, inputManager) { }
        ~Game() {
            cleanup();
        }

    private:
        /* Primary Game API */
        void setup() override;
        void cleanup() override;
        void loop() override;
        void handleJoystickChanged(Vec2 vec) override;
        void handleCollision(GameObject* obj1, GameObject* obj2, util::Point& contact) override;
        void handleButtonPressed1() override;
        void handleButtonReleased1() override;
        void handleButtonPressed2() override;
        void handleButtonReleased2() override;

};

#endif