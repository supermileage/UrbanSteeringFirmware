#include "stdio.h"
#include "mbed.h"
#include "SPI_TFT_ILI9341.h"
#include "Arial12x12.h"
#include "supermileagelogo.h"
#include "canID.h"
#include <string>  
#include "main.h"

DigitalOut myled(LED1);
Serial pc(USBTX, USBRX);
CAN can(D10, D2); 
Ticker throttle;

// !! Need to figure out interrupt priority
InterruptIn ignition(D6);
InterruptIn horn(D5);
InterruptIn leftBlinker(D4);
InterruptIn rightBlinker(D3);
InterruptIn hazards(D9);
InterruptIn wipers(A2);

AnalogIn dms(A1); //deadman switch
Timer debounce;


// the TFT is connected to SPI pin 5-7
SPI_TFT_ILI9341 TFT(D11, D12, D13, A7, D0, A3,"TFT"); 
// mosi, miso, sclk, cs, reset, dc

int main() {

    startUpSeq();

    //Setup ignition interrupters
    ignition.rise(&startCar);
    ignition.fall(&stopCar);

    //Setup accessories interrupters
    setupAccessoriesInterrupts();
    
    // Set up ticker for checking throttle @ freq in Hz
    throttle.attach(&checkThrottle, 1/FREQ);

    setupPriority();

    while(1){
        // just do something random to make sure code doesn't end
        int k = 0;
    }

}

void setupPriority(){
    
}

// Checks DMS analog value to see if it hand is on wheel
// dmsTHRESH value requires tuning
void checkDMS(){

    dmsVAL = dms.read();

    if (dmsVAL >= dmsTHRESH){
        DMS = 1;
    }
    else if (dmsVAL < dmsTHRESH){
        DMS = 0;
    }
}

// IGNITION flag on when switch turns on
void startCar(){
    IGNITION = 1;
    // elapsed time start here? for printing to display
    // send CAN msg to start (maybe)
    // dependent on motor controller
    
}

// IGNITION flag off when switch off; immediately send stop data to motor to ensure motor isn't on
void stopCar(){
    IGNITION = 0;
    // send CAN msg to motor ID to stop
    const char* data = "some stop data sequence";     
    can.write(CANMessage(0,data,8));
}


// read throttle from can and send to motor 
void checkThrottle(){
   
    // read dms and ignition
    checkDMS();
    
    if(DMS && IGNITION){
        // Read potentiometer from pedal and relay to motor
        CANMessage msg;

        //read analog in of pot (comes in as 0-1)
        if(can.read(msg)){
            
            // Parse pot data and write to motor id
            
            const char* data = "01010100";
            can.write(CANMessage(0,data,8));

        }
    }
    else if(IGNITION){
        TFT.locate(100,100);
        TFT.printf("Waiting for hand..."); 
    }
    else{
        TFT.locate(100,100);
        TFT.printf("Waiting for ignition and DMS...");
    }
    
    // print data eg power, voltage, # of lap, elapsed time

}

void setupAccessoriesInterrupts(){

    debounce.start();

    leftBlinker.rise(&controlAccessories);
    rightBlinker.rise(&controlAccessories);
    horn.rise(&controlAccessories);
    hazards.rise(&controlAccessories);
    wipers.rise(&controlAccessories);
    
    leftBlinker.fall(&controlAccessories);
    rightBlinker.fall(&controlAccessories);
    horn.fall(&controlAccessories);
    hazards.fall(&controlAccessories);
    wipers.fall(&controlAccessories);

}

// reads digital inputs and sends CAN msg to accessories upon any interrupts
void controlAccessories(){
    
    // read digial input for interrupters
    // Pack one string for can msg
    // bit sequence: leftBlinker, rightBlinker, horn, hazards, wipers
    string leftbVal = to_string(leftBlinker.read());
    string rightbVal = to_string(rightBlinker.read());
    string hornVal = to_string(horn.read());
    string hazardVal = to_string(hazards.read());
    string wiperVal = to_string(wipers.read());
    
    // "00110" = 6    

    // some example sequence
    const char* data =  "00110011";

    // debounce and send data
    if (debounce.read_ms()>200){
        
        // currently have arbitrary CAN ID of 1 and length 8
        can.write(CANMessage(1,data,8));

        debounce.reset();
    }    
    
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


