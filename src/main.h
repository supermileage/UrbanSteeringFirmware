#ifndef _MAIN_H // include guard
#define _MAIN_H

//Button Definitions
#define JOYSTICK_BUTTON 0
#define IND_RIGHT_BUTTON 2
#define IND_LEFT_BUTTON 1
#define WIPER_BUTTON 3
#define LIGHTS_BUTTON 4
#define HORN_BUTTON 5
#define IGNITION_BUTTON 6
#define HAZARDS_BUTTON 7

//LED definitions
#define WIPER_LED 1
#define LIGHTS_LED 2
#define HORN_LED 3
#define IGNITION_OFF_LED 4
#define IGNITION_ON_LED 5
#define HAZARDS_LED 6

#include "stdio.h"
#include "mbed.h"


/**
 * @brief handles all accessories-related tasks
 */
void handle_accessories();

/**
 * @brief Called when any button on steering is switched on/off
 * 
 * @return char bit shifted on/off values for each accessory (sent to CanAccessories board in Op Mode 0)
 */
char read_accessory_inputs(char& hazardsVal);

/**
 * @brief Processes current state of throttle, depending on ignition, brakes and dms.  Will send a CAN message 
 * to motor controller with throttle data, and another to telemetry indicating states of ignition, dms, etc..
 * 
 */
void handle_motor_inputs();

/**
 * @brief Read dead man's switch
 * 
 * @return bool indicating whether dead man's switch is on/off
 */
data_t getDmsVal();

/**
 * @brief Gets the value of throttle as unsigned char (0 <= value <= 255)
 * 
 * @return unsigned char the current throttle value
 */
unsigned char get_throttle_val();

/**
 * @brief background thread task which updates lcd display
 */
void runSteeringDisplay();

/**
 * @brief reads incoming can messages and updates speed and battery values for display
 */
void receive_can();

/**
 * @brief Initializes LCD display
 * 
 */
void initializeDisplay();

/**
 * @brief checks for and handles joystick button actions
 */
void handleButton();

void setLedState();

void updateShiftRegs();

void blinkHazardLed();

#endif