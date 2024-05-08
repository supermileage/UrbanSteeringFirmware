#include "main.h"

#include <mbed.h>
#include <stdio.h>

#include <chrono>
#include <cmath>
#include <string>

#include "SPI_TFT_ILI9341.h"
#include "SharedProperty.h"
#include "SteeringDisplay.h"
#include "can_common.h"
#include "rtos.h"

using namespace std;
using namespace std::chrono;

// #define DEBUG_MODE
#define DMS_DELTA_THRESHOLD 500
#define MIN_THROTTLE_INPUT 3000
#define MAX_THROTTLE_INPUT 7000

#define ACCESSORIES_TRANSMIT_INTERVAL 50
#define MOTOR_CONTROLLER_TRANSMIT_INTERVAL 100
#define DEBOUNCE_TIME 50
#define GESTURE_MARGIN 500

#define MIN_THROTTLE_OUTPUT 0
#define MAX_THROTTLE_OUTPUT 255
#define THROTTLE_RANGE (MAX_THROTTLE_INPUT - MIN_THROTTLE_INPUT)

#define CAN_BATT_SOC_SCALING_FACTOR 2.0
#define CAN_BATT_VOLTAGE_SCALING_FACTOR 10.0

SPI_TFT_ILI9341 TFT(D11, D12, D13, D9, D0, A4);
DigitalOut sdCs(A0);
CAN can(D10, D2, 500000);
SteeringDisplay display(&TFT);

#ifdef DEBUG_MODE
BufferedSerial pc(USBTX, USBRX);
#endif

// Accessories
DigitalIn brake(D1, PullUp);


// Ready
AnalogIn dms(A1);
AnalogIn throttle(A6);
DigitalOut dmsLed(A5);

// shift registers
DigitalOut shiftClk(D3);
DigitalOut ledOut(D4);
DigitalOut shiftLatch(D5);
DigitalIn buttonIn(D6);

// shift reg
bool buttonState[8] = {}; 
bool ledState[8] = {};

// Joystick
AnalogIn joyX(A3);
AnalogIn joyY(A2);

Timer timerMotor;
Timer clockResetTimer;
Timer timerAccessories;

bool lastHazards = false;
Ticker timerFlash;

// Properties to bind to steering GUI and CAN events
SharedProperty<data_t> dmsVal(0);
SharedProperty<data_t> ignitionVal(0);
SharedProperty<data_t> brakeVal(0);
SharedProperty<batt_t> batterySocVal(0);
SharedProperty<batt_t> batteryVoltageVal(0);
SharedProperty<speed_t> currentSpeedVal(0);
SharedProperty<throttle_t> throttleVal(0);
SharedProperty<rpm_t> rpmVal(0);
SharedProperty<eshift_t> eShiftVal(0);
SharedProperty<data_t> lightsVal(0);
SharedProperty<data_t> turnLeftVal(0);
SharedProperty<data_t> turnRightVal(0);
SharedProperty<data_t> prevAccVal(0);
SharedProperty<data_t> lastGesture(0);
SharedProperty<data_t> blink(0);
SharedProperty<steering_time_t> timeVal(steering_time_t{0, 0});

// global variables shared between main and display threads
int64_t g_lastTime = 0;
int counter = 0;
int eshift = 1;
int prev_state = 0; // 0 neutral, 1 up, -1 down

void initializeDisplay() {
    // initialize
    display.init();

    // add data display bindings
    display.addDynamicGraphicBinding(dmsVal, SteeringDisplay::Dms);
    display.addDynamicGraphicBinding(ignitionVal, SteeringDisplay::Ignition);
    display.addDynamicGraphicBinding(brakeVal, SteeringDisplay::Brake);
    display.addDynamicGraphicBinding(batterySocVal, SteeringDisplay::Soc);
    display.addDynamicGraphicBinding(batteryVoltageVal, SteeringDisplay::Voltage);
    display.addDynamicGraphicBinding(currentSpeedVal, SteeringDisplay::Speed);
    display.addDynamicGraphicBinding(throttleVal, SteeringDisplay::Power);
    display.addDynamicGraphicBinding(rpmVal, SteeringDisplay::Rpm);
    display.addDynamicGraphicBinding(eShiftVal, SteeringDisplay::eShift);
    display.addDynamicGraphicBinding(lightsVal, SteeringDisplay::Lights);
    display.addDynamicGraphicBinding(turnLeftVal, SteeringDisplay::LeftSignal);
    display.addDynamicGraphicBinding(turnRightVal, SteeringDisplay::RightSignal);
    display.addDynamicGraphicBinding(timeVal, SteeringDisplay::Minutes);
    display.addDynamicGraphicBinding(blink, SteeringDisplay::Hazards);
}

int main() {
    timerMotor.start();
    clockResetTimer.start();
    timerAccessories.start();

    shiftClk.write(0);
    ledOut.write(0);
    shiftLatch.write(0);
    sdCs.write(1);

    // Initalize Accessories--it's impossible for left and right blinker to be on simultaneously so this will cause can update to be sent on boot
    prevAccVal.set(0xFF);

    initializeDisplay();

    Thread display_thread;
    display_thread.start(runSteeringDisplay);

    dmsLed.write(0);

    while (1) {
        handleTime();
        handle_accessories();
        handle_motor_inputs(eshift, prev_state);
        receive_can();
        updateShiftRegs();
        setLedState();
        blink.set(ledState[HAZARDS_LED]);
    }
}

void handle_accessories() {
    if (duration_cast<milliseconds>(timerAccessories.elapsed_time()).count() > ACCESSORIES_TRANSMIT_INTERVAL) {
        char hazardsOn;
        char currentAcc = read_accessory_inputs(hazardsOn);
        if ((prevAccVal.value() != currentAcc)) {
            // turn hazards off
            if (hazardsOn) {
                const unsigned char data[] = {0x2, 0x4 << 1, 0x5 << 1};
                can.write(CANMessage(CAN_ACC_OPERATION, data, 3));
                wait_us(1000);
            }
            const char data[] = {0, currentAcc};
            can.write(CANMessage(CAN_ACC_OPERATION, data, 2));
            prevAccVal.set(currentAcc);
        }
        timerAccessories.reset();
    }
}

char read_accessory_inputs(char &hazards) {
    lightsVal.set(buttonState[LIGHTS_BUTTON]);
    char currentBrake = (char)(!brake.read());
    char horn = buttonState[HORN_BUTTON];
    hazards = buttonState[HAZARDS_BUTTON];
    char turnLeft = buttonState[IND_LEFT_BUTTON];
    char turnRight = buttonState[IND_RIGHT_BUTTON];
    char wiperVal = buttonState[WIPER_BUTTON];

    if (hazards) {
        turnLeftVal.set(1);
        turnRightVal.set(1);
    } else {
        turnLeftVal.set(turnLeft);
        turnRightVal.set(turnRight);
    }
    char dataStr = (wiperVal << 6) | (turnLeftVal.value() << 5) | (turnRightVal.value() << 4) |
                   (hazards << 3) | (horn << 2) | (currentBrake << 1) | lightsVal.value();

    if (hazards) {
        turnLeftVal.set(0);
        turnRightVal.set(0);
    }
    return dataStr;
}

void handle_motor_inputs(int &eshift, int &prev_state) {
    if (duration_cast<milliseconds>(timerMotor.elapsed_time()).count() > MOTOR_CONTROLLER_TRANSMIT_INTERVAL) {
        dmsVal.set(getDmsVal());
        ignitionVal.set(buttonState[IGNITION_BUTTON]);
        brakeVal.set((char)!brake.read());

        throttle_t currentThrottle = get_throttle_val();

        if (!dmsVal.value() || !ignitionVal.value() || brakeVal.value()) {
            currentThrottle = 0;
        }

        int curr_state;
        int Joystick_x = (int)(joyX.read()*10000);
        // printf("voltage= %04d\n",Joystick_x);

        if (Joystick_x > 9000){
            curr_state = -1;
        } else if( Joystick_x < 1000){
            curr_state = 1;
        } else{
            curr_state = 0;
        }

        if (prev_state == 0){
            if(curr_state == 1){
                eshift++;
            } else if(curr_state == -1){
                eshift--;
            }
        }
        
        prev_state = curr_state;
        
        // Limit eshift range
        if(eshift < 1){
            eshift = 1;
        } else if(eshift > 5){
            eshift = 5;
        } 

        // eShift value is displayed
        eShiftVal.set(eshift);

        // eShift modulates the range of throttle provided to the motor controller
        switch(eshift){
            case 1:
                currentThrottle = currentThrottle/5;
                break;
            case 2:
                currentThrottle = currentThrottle/2.5;
                break;
            case 3:
                currentThrottle = currentThrottle/1.66;
                break;
            case 4:
                currentThrottle = currentThrottle/1.25;
                break;
            case 5:
                currentThrottle = currentThrottle;
                break;
            default:
                currentThrottle = 0;
                break;
        }

        throttleVal.set(currentThrottle);

        // Throttle Data
        const throttle_t throttleData[] = {throttleVal.value()}; 
        can.write(CANMessage(CAN_STEERING_THROTTLE, throttleData, 1));

        // Ready Data
        char ready = (brakeVal.value() << 2) | (dmsVal.value() << 1) | ignitionVal.value();
        const char readyData[] = {ready};
        can.write(CANMessage(CAN_STEERING_READY, readyData, 1));

        timerMotor.reset();
    }
}

throttle_t get_throttle_val() {
    int throttleVal = (int)(throttle.read() * 10000);

#ifdef DEBUG_MODE
    printf("Throttle Input: %04d - ", throttleVal);
#endif

    // Keep values within min/max
    if (throttleVal <= MIN_THROTTLE_INPUT) {
        throttleVal = MIN_THROTTLE_INPUT;
    } else if (throttleVal >= MAX_THROTTLE_INPUT) {
        throttleVal = MAX_THROTTLE_INPUT;
    }

    // Normalize throttle value
    throttleVal -= MIN_THROTTLE_INPUT;

    // Scale for output
    throttle_t throttleOut = (throttleVal * MAX_THROTTLE_OUTPUT) / THROTTLE_RANGE;

    return throttleOut;
}

data_t getDmsVal() {
    int dmsCtrl = (int)(dms.read() * 10000);
    dmsLed.write(1);
    wait_us(40);
    int dmsVal = (int)(dms.read() * 10000);
    dmsLed.write(0);
    int dmsDelta = dmsVal - dmsCtrl;
#ifdef DEBUG_MODE
    printf("DMS Delta: %04d\n", dmsDelta >= 0 ? dmsDelta : 0);
#endif

    return (data_t)(dmsDelta > DMS_DELTA_THRESHOLD);
}

// Checks for CAN updates
void receive_can() {
    CANMessage msg;

    if (can.read(msg)) {
        // if (msg.id == CAN_TELEMETRY_GPS_DATA) {
        //     currentSpeedVal.set((speed_t)msg.data[0]);
            
        // } else 
        if(msg.id == CAN_MOTOR_RPM) {   
            // Reconstruct the integer value from the byte array
            int rpm =(msg.data[0] << 8) | msg.data[1];
            rpmVal.set(rpm);
            currentSpeedVal.set(rpm/16/3);
            
        } else if (msg.id == CAN_ORIONBMS_PACK) {
            uint8_t socData = msg.data[4];
            batt_t soc = socData / CAN_BATT_SOC_SCALING_FACTOR;
            batterySocVal.set(soc);
            uint16_t voltageData = msg.data[1] | msg.data[0] << 8;
            batt_t voltage = voltageData / CAN_BATT_VOLTAGE_SCALING_FACTOR;
            batteryVoltageVal.set(voltage);
        } 
        
    }
}

void handleTime() {
    if (!buttonState[JOYSTICK_BUTTON]) {
        clockResetTimer.reset();
    }

    int64_t currentTime = clockResetTimer.read_ms();

    timeVal.set(steering_time_t{(int)currentTime / 1000 / 60, (int)currentTime / 1000 % 60});
}

void runSteeringDisplay() {
    while (1) {
        display.run();
    }
}

void updateShiftRegs() {
    shiftLatch.write(1);
    for (int i = 7; i >= 0; i--) {
        buttonState[i] = buttonIn.read();
        shiftClk.write(1);
        shiftClk.write(0);
    }
    shiftLatch.write(0);
    for (int i = 8; i >= 0; i--) {
        ledOut.write(!ledState[i]);
        shiftClk.write(1);
        shiftClk.write(0);
    }
}

void blinkHazardLed() {
    ledState[HAZARDS_LED] = !ledState[HAZARDS_LED];
}

void setLedState() {
    // Set wiper LED
    ledState[WIPER_LED] = buttonState[WIPER_BUTTON];
    // Set lights LED
    ledState[LIGHTS_LED] = buttonState[LIGHTS_BUTTON];
    // Set Horn LED
    ledState[HORN_LED] = buttonState[HORN_BUTTON];
    // Set Ignition LED
    ledState[IGNITION_OFF_LED] = !buttonState[IGNITION_BUTTON];
    ledState[IGNITION_ON_LED] = buttonState[IGNITION_BUTTON];
    // Flash Hazards LED
    if (buttonState[HAZARDS_BUTTON] != lastHazards) {
        if (buttonState[HAZARDS_BUTTON]) {
            timerFlash.attach(blinkHazardLed, 500ms);
            ledState[HAZARDS_LED] = true;
        } else {
            timerFlash.detach();
            ledState[HAZARDS_LED] = false;
        }
        lastHazards = buttonState[HAZARDS_BUTTON];
    }
}