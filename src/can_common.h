#ifndef _CAN_COMMON_H_
#define _CAN_COMMON_H_

// Steering
// 0x0 - 0x5F
// 0x100 - 0x15F
#define CAN_STEERING_THROTTLE   0x01
#define CAN_STEERING_STATUS     0x02
#define CAN_STR_ACC             0xA

// Accessories
// 0x60 - 0x7F
// 0x160 - 0x17F
#define CAN_ACC_OPERATION       0x60
#define CAN_ACC_STATUS          0x71

#define STATUS_HEADLIGHTS		0x0
#define STATUS_BRAKELIGHTS		0x1
#define STATUS_HORN				0x2
#define STATUS_HAZARDS			0x3
#define STATUS_RIGHT_SIGNAL		0x4
#define STATUS_LEFT_SIGNAL		0x5
#define STATUS_WIPERS			0x6
#define STATUS_NULL				0xFF

// Telemetry
// 0x80 - 0x9F
// 0x180 - 0x19F
#define CAN_TELEMETRY_GPS_SPEED 0x80

// BMS
// 0x201 - 0x23F
// 0x241 - 0x27F
#define CAN_BMS_REQUEST         0x201
#define CAN_BMS_RESPONSE        0x241

#endif
