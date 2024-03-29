#ifndef _SHARED_PROPERTY_H_
#define _SHARED_PROPERTY_H_

#include <string>
#include <vector>
#include <Mutex.h>

#include "util.h"

using namespace util;

/**
 * @brief Defines a property whose value is shared between arbitrary objects / threads
 * 
 * @tparam T
 */
template <class T>
class SharedProperty {
	public:
		SharedProperty(T value) : _value(value) { }

		~SharedProperty() {
			for (Command* command : _valueChangedDelegates) {
				delete command;
			}
		}

		void set(T value) {
			_stateMutex.lock();
			if (value != _value) {
				_value = value;
				_stateMutex.unlock();
				
				_onValueChanged();
			} else {
				_stateMutex.unlock();
			}
		}

		T value() {
			_stateMutex.lock();
			T ret = _value;
			_stateMutex.unlock();

			return ret;
		}

		void addValueChangedListener(Command* command) {
			_valueChangedDelegates.push_back(command);
		}

		void removeValueChangedListener(Command* command) {
			std::vector<Command*>::iterator it = std::find(_valueChangedDelegates.begin(), _valueChangedDelegates.end(), command);

			if (it != _valueChangedDelegates.end()) {
				_valueChangedDelegates.erase(std::remove(_valueChangedDelegates.begin(), _valueChangedDelegates.end(), command),
					_valueChangedDelegates.end());
			}
		}

	private:
		T _value;
		Mutex _stateMutex;
		std::vector<Command*> _valueChangedDelegates;

		void _onValueChanged() {
			for (Command* command : _valueChangedDelegates) {
				command->execute((CommandArgs)&_value);
			}
		}
};

#endif