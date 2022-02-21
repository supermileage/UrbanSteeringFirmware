#ifndef _MAIN_H // include guard
#define _MAIN_H

#include "stdio.h"
#include "mbed.h"

void setupAccessoriesInterrupts();
void startUpSeq();
void controlAccessories();
void checkThrottle();
void startCar();
void stopCar();
void checkDMS();
void setupPriority();

//init variables
double FREQ = 100;
int HIGH = 1;
int LOW = 0;
int IGNITION = 0;
double dmsVAL = 0;
int DMS = 0;
int dmsTHRESH = 500;

#endif