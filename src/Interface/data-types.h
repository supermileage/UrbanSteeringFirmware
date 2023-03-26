#pragma once

/* macro for shared property type */
#define batt_t float
#define data_t char
#define speed_t unsigned
#define throttle_t uint8_t

struct steering_time_t { 
	int minutes;
	int seconds;

	bool operator==(const steering_time_t& rhs) {
		return minutes == rhs.minutes && seconds == rhs.seconds;
	}

	bool operator!=(const steering_time_t& rhs) {
		return minutes != rhs.minutes || seconds != rhs.seconds;
	}
};