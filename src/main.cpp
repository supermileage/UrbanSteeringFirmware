#include "stdio.h"
#include "mbed.h"
#include "SPI_TFT_ILI9341.h"
#include "Arial12x12.h"
#include "supermileagelogo.h"
#include <string>  
#include "can_common.h"
#include "main.h"

int main() {
    startUpSeq();
    timer_MTR.start();
    timer_TEL.start();
    timer_ACC.start();
    char prev_data_str  = ACC_task();
    char curr_data_str  = 0;
    CANMessage msg;
    const char data[] = {0b11111111,0};
    can.write(CANMessage(0x80,data,2)); 

    while(1){
        can.read(msg);
        if (timer_ACC.read()> 0.2){
            curr_data_str = ACC_task();
            if ((curr_data_str ^ prev_data_str)){
                const char data[] = {0, curr_data_str};
                can.write(CANMessage(CAN_STR_ACC,data,2));
                prev_data_str = curr_data_str;
            }
            timer_ACC.reset();
        }
        if (timer_MTR.read()> 1){
            MTR_task();
            timer_MTR.reset();
        }
    }
   
}

char ACC_task(void){

    char lightsVal  =    (char) (lights.read());
    char hornVal    =    (char) (horn.read()<<2);
    char hazardVal  =    (char) (hazards.read()<<3);
    char rightbVal  =    (char) (rightBlinker.read()<<4);
    char leftbVal   =    (char) (leftBlinker.read()<<5);
    char wiperVal   =    (char) (wipers.read()<<6);   

    if (hazardVal == 0x8){
        leftbVal    = 0x10;
        rightbVal   = 0x20;
    }
    char data_str  =  lightsVal | leftbVal | rightbVal | hornVal | hazardVal | wiperVal;
    return data_str;
}
int readDMS(){
    float DMS_VAL = dms.read();
    int DMS_FLAG =0;
    if (DMS_VAL <= DMS_THRESH){
        DMS_FLAG = 1;
    }
    else if (DMS_VAL > DMS_THRESH){
        DMS_FLAG = 0;
    }
    return DMS_FLAG;
}

void MTR_task(void){
    int DMS_FLAG    = readDMS();
    int IGNITION    = ignition.read();
    int BRAKE       = brake.read();
    char ignitionVal    =    (char) (IGNITION);
    char brakeVal       =    (char) (BRAKE<<1);
    char DMSVal         =    (char) (DMS_FLAG<<2);
    float throttleVal   = 0;
    
    if (DMS_FLAG && IGNITION && !BRAKE){
        throttleVal = throttle.read();
    }   
    
    else {
        throttleVal = 0;
    }
    std::string tmp = std::to_string(throttleVal);
    const char* throttleCANData = tmp.c_str();
    //can.write(CANMessage(CAN_STEERING_THROTTLE,throttleCANData,8));
    char data_str   = DMSVal | ignitionVal | brakeVal;
    const char data[] = {0, data_str};
    //float throttlevalue = throttle.read();
    //pc.printf("DMS  = %lf , FLAG = %d \n", throttlevalue , DMS_FLAG );
    can.write(CANMessage(CAN_STEERING_THROTTLE,data,2));

}
void startUpSeq(){

     TFT.set_orientation(3);
     TFT.background(Black);
    TFT.cls(); 
    TFT.set_font((unsigned char*) Arial12x12);
    TFT.locate(100,100);
    // print supermileage logo
    TFT.Bitmap(55,78,201,85,supermileagelogo);
}