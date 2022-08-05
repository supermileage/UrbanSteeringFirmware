#ifndef _INPUT_HANDLER_H_
#define _INPUT_HANDLER_H_

#include <unordered_map>

#include "util.h"
#include "Input.h"

/**
 * @brief Receives input from gpio peripherals and responds accordingly
 */
class InputHandler {
	public:
		~InputHandler() { }
		virtual void run() = 0;
		virtual void addInput(Input* input, util::Command* behavior = NULL) = 0;

};

#endif