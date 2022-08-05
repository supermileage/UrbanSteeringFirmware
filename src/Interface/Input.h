#ifndef _INPUT_H_
#define _INPUT_H_

#include <vector>
#include <stdint.h>

class Input {
	public:
		Input() { }
		~Input() { }
		virtual bool read() = 0;
		const std::vector<uint8_t>& getData() {
			return _data;
		}

	protected:
		std::vector<uint8_t> _data;

};

#endif