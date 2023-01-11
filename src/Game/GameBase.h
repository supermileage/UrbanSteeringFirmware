#ifndef _GAME_BASE_H_
#define _GAME_BASE_H_

#include "mbed.h"
#include "config.h"
#include "SPI_TFT_ILI9341.h"
#include "Timer.h"
#include "util.h"
#include "InputManager.h"
#include "ThreadedQueue.h"
#include "GameObject.h"

#define WINDOW_WIDTH 320
#define WINDOW_HEIGHT 240
#define FRAME_RATE_MILLIS 10

class InputManager;

class GameBase {
    public:
        GameBase(SPI_TFT_ILI9341* tft, InputManager* _inputManager);
        virtual ~GameBase();
        void init();
        int run();

    protected:
        SPI_TFT_ILI9341* _tft;
        Timer* _timer;
        Vec2 _lastJoystickPos = { 0, 0 };
        int64_t _lastFrameMillis = 0;
        int64_t _deltaT = 0;
        bool _run = true;

        virtual void setup() = 0;
        virtual void cleanup() = 0;
        virtual void loop() = 0;
        virtual void handleJoystickChanged(Vec2 vec) = 0;
        virtual void handleCollision(GameObject* obj1, GameObject* obj2, util::Point& contact) = 0;
        virtual void handleButtonPressed1() = 0;
        virtual void handleButtonReleased1() = 0;
        virtual void handleButtonPressed2() = 0;
        virtual void handleButtonReleased2() = 0;
        int64_t millis();
        int64_t micros();
        void draw(GameObject* graphic);

    private:
        static GameBase* _currentGame;
        InputManager* _inputManager;
        Command* _collisionDelegate = nullptr;
        Thread _renderThread;
        ThreadedQueue<GameObject*> _renderQueue;
        
        void _handleCloseRequest();
        void _handleCollision(Collider::Collision collision);
        static void _runRenderQueue();
};

#endif