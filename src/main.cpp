#include "stdio.h"
#include "mbed.h"
#include "SPI_TFT_ILI9341.h"
#include "Arial12x12.h"
#include "supermileagelogo.h"
#include "canID.h"
#include <string>  
#include "main.h"

DigitalOut myled(LED3);
CAN can(D10, D2, 500000); 

InterruptIn ignition(D6);
InterruptIn horn(D5);
InterruptIn leftBlinker(D4);
InterruptIn rightBlinker(D3);
InterruptIn hazards(D9);
InterruptIn wipers(A2);
AnalogIn dms(A1); //deadman switch

SPI_TFT_ILI9341 TFT(D11, D12, D13, A7, D0, A3,"TFT"); 

EventQueue queue;

int main() {   
    startUpSeq(); 
    myled = 0;  
    const char data[] = {0b11111111,0};
    can.write(CANMessage(0x80,data,2));
       
    Thread eventThread;
    eventThread.start(callback(&queue, &EventQueue::dispatch_forever));
    setupAccessoriesInterrupts();
    while(1){  
        myled = !myled;
        wait(0.25);         
    }
}

void setupAccessoriesInterrupts(){

    leftBlinker.rise(queue.event(&controlAccessories));
    rightBlinker.rise(queue.event(&controlAccessories));
    horn.rise(queue.event(&controlAccessories));
    hazards.rise(queue.event(&controlAccessories));
    wipers.rise(queue.event(&controlAccessories));
    
    leftBlinker.fall(queue.event(&controlAccessories));
    rightBlinker.fall(queue.event(&controlAccessories));
    horn.fall(queue.event(&controlAccessories));
    hazards.fall(queue.event(&controlAccessories));
    wipers.fall(queue.event(&controlAccessories));
}

// reads digital inputs and sends CAN msg to accessories upon any interrupts
void controlAccessories(){
    
    char hornVal = (char)(horn.read()<<2);
    char hazardVal = (char)(hazards.read()<<3);
    char rightbVal = (char)(rightBlinker.read()<<4);
    char leftbVal = (char)(leftBlinker.read()<<5);
    char wiperVal = (char)(wipers.read()<<6);
        
    char data_str  = leftbVal | rightbVal | hornVal | hazardVal |wiperVal;
    const char data[] = {data_str,0};
    can.write(CANMessage(0x80,data,2));
}  

// Print some startup sequence
void startUpSeq(){

     TFT.set_orientation(3);
     TFT.background(Black);
    TFT.cls(); 
    TFT.set_font((unsigned char*) Arial12x12);
    TFT.locate(100,100);
    // print supermileage logo
    TFT.Bitmap(40,90,201,85,supermileagelogo);
}


