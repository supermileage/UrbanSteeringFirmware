#ifndef _MAIN_H // include guard
#define _MAIN_H

#include "stdio.h"
#include "mbed.h"
CAN can(D10, D2, 500000);

DigitalIn horn(D5,PullDown);  //PB6
DigitalIn leftBlinker(D3,PullDown); //PB0
DigitalIn rightBlinker(D4,PullDown); //PB7
DigitalIn hazards(D9,PullDown); // PA8
DigitalIn wipers(A2,PullDown); // PA3
DigitalIn lights(A0,PullDown); //
DigitalIn ignition(D6,PullDown);
AnalogIn dms(A1); //deadman switch
AnalogIn throttle(A6);
DigitalIn brake(D1,PullUp);
//SPI_TFT_ILI9341 TFT(D11, D12, D13, A7, D0, A3,"TFT"); 
Serial pc(USBTX, USBRX); // tx, rx

Timer timer_MTR;
Timer timer_TEL;
Timer timer_ACC;

#define DMS_THRESH 0.06

char ACC_task(void);
void MTR_task(void);
int readDMS(void);
//void startUpSeq(void);

#endif