#ifndef _MAIN_H // include guard
#define _MAIN_H

#include "stdio.h"
#include "mbed.h"

/**
 * @brief Called when any button on steering is switched on/off
 * 
 * @return char bit shifted on/off values for each accessory (sent to CanAccessories board in Op Mode 0)
 */
char ACC_task();

/**
 * @brief Processes current state of throttle, depending on ignition, brakes and dms.  Will send a CAN message 
 * to motor controller with throttle data, and another to telemetry indicating states of ignition, dms, etc..
 * 
 */
void MTR_task();

/**
 * @brief Read dead man's switch
 * 
 * @return bool indicating whether dead man's switch is on/off
 */
char get_dms_val();

/**
 * @brief Gets the value of throttle as unsigned char (0 <= value <= 255)
 * 
 * @return unsigned char the current throttle value
 */
unsigned char get_throttle_val();

/**
 * @brief background thread task which updates lcd display
 * 
 * @param arg unused task arg
 */
void display_task(const void* arg);

/**
 * @brief Initializes LCD display
 * 
 */
void initialize_display();

#endif