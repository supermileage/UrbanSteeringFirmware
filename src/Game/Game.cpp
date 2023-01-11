#include "Game.h"

/* Add includes for any graphics/bitmaps you want to use */
#include "Circle.h"
#include "Rectangle.h"

#define BALL_SPEED 7
#define BALL_RADIUS 10

#define PADDLE_NAME "paddle"
#define PADDLE_SPEED 15
#define PADDLE_WIDTH 50
#define PADDLE_HEIGHT 50
#define PADDLE_START_X WINDOW_WIDTH - PADDLE_WIDTH * 2
#define PADDLE_START_Y WINDOW_HEIGHT / 2 - PADDLE_HEIGHT / 2
#define PADDLE_START util::Point { WINDOW_WIDTH / 2 - 25, WINDOW_HEIGHT / 2 - 25 }

/* Declare any global game objects here */
Rectangle* paddle;
Circle* ball1;
Circle* ball2;
Circle* ball3;
std::vector<Circle*> balls;

/* Game setup */
void Game::setup() {
    // Initialize any game objects with their start positions here.  Assume that _tft->width()
    // and _tft->height() may vary, so try to avoid using hard-coded positions (ie. you should able
    // to play your game on different-sized screens / windows)

    // example code:
    paddle = new Rectangle("paddle", true);
    paddle->init(_tft, PADDLE_START, PADDLE_WIDTH, PADDLE_HEIGHT, PurpleTFT, true);
    paddle->setDirection(Vec2 { 0, 0 });
    paddle->setSpeed(PADDLE_SPEED);
    draw(paddle);

    ball1 = new Circle("ball1", true);
    ball1->init(_tft, 60, _tft->height() - 50, BlueTFT, BALL_RADIUS + 10, true);
    ball1->setDirection(Vec2 { 1.7, -1 });
    ball1->setSpeed(BALL_SPEED - 5);
    balls.push_back(ball1);
    draw(ball1);

    ball2 = new Circle("ball2", true);
    ball2->init(_tft, BALL_RADIUS + 10, BALL_RADIUS + 10, YellowTFT, BALL_RADIUS, true);
    ball2->setDirection(Vec2 { 1, 1 });
    ball2->setSpeed(BALL_SPEED + 3);
    balls.push_back(ball2);
    draw(ball2);

    ball3 = new Circle("ball3", true);
    ball3->init(_tft, _tft->width() - 50, 50, GreenTFT, BALL_RADIUS + 20, true);
    ball3->setDirection(Vec2 { -1, -1.5 });
    ball3->setSpeed(BALL_SPEED + 2);
    balls.push_back(ball3);
    draw(ball3);
}

void Game::cleanup() {
    delete paddle;
    for (Circle* ball : balls) {
        delete ball;
    }
    balls.clear();
}

/* Game loop */
void Game::loop() {
    // example
    for (Circle* ball : balls) {
        if (ball->getPosition().x + ball->getRadius() < 0 || 
            ball->getPosition().x + ball->getRadius() > _tft->width()) {
            ball->setDirection(Vec2 { -ball->getDirection().x, ball->getDirection().y });
        }
        if (ball->getPosition().y + ball->getRadius() < 0 ||
            ball->getPosition().y + ball->getRadius() > _tft->height()) {
            ball->setDirection(Vec2 { ball->getDirection().x, -ball->getDirection().y });
        }
        if (ball->move()) {
            draw(ball);
        }
    }

    paddle->move();
    draw(paddle);
}

/* Joystick changed event */
void Game::handleJoystickChanged(Vec2 vec) {
    paddle->setDirection(Vec2 { 0, vec.y } );
}

/* Collision event -- params are game objects which have collided, contact is point of collision (on obj1) */
void Game::handleCollision(GameObject* obj1, GameObject* obj2, util::Point& contact) {
    if (obj1->getName().compare(PADDLE_NAME) == 0) {
        if (contact.x == paddle->getPosition().x || contact.x == paddle->getPosition().x + paddle->getWidth()) {
            obj2->setDirection(Vec2 { -obj2->getDirection().x, obj2->getDirection().y });
        } else {
            obj2->setDirection(Vec2 { obj2->getDirection().x, -obj2->getDirection().y });
        }
        return;
    }

    Vec2 vec1 = obj1->getDirection();
    Vec2 vec2 = obj2->getDirection();
    obj1->setDirection(Vec2 { vec2.x, vec2.y });
    obj2->setDirection(Vec2 { vec1.x, vec1.y });
}

/* Button 1 pressed event */
void Game::handleButtonPressed1() {
    
}

/* Button 1 released event */
void Game::handleButtonReleased1() {
    
}

/* Button 2 pressed event */
void Game::handleButtonPressed2() {
    
}

/* Button 2 released event */
void Game::handleButtonReleased2() {
    
}