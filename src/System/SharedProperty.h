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
			for (Command* command : _valueChangedEventCommands) {
				delete command;
			}
		}

		T operator=(T other) {
			setValue(other);
			return _value;
		}

		SharedProperty<T>& operator=(SharedProperty<T>& other) {
			T val = other.getValue();
			setValue(val);
			return *this;
		}

		friend std::ostream& operator<<(std::ostream& os, SharedProperty<T>& property) {
			os << property.getValue();
			return os;
		}

		T operator<<(int32_t shift) {
			return (T)(_value << shift);
		}

		T operator!(SharedProperty<T> property) {
			return !property.getValue();
		}

		void setValue(T value) {
			_stateMutex.lock();
			if (value != _value) {
				_value = value;
				_stateMutex.unlock();
				
				_onValueChanged();
			} else {
				_stateMutex.unlock();
			}
		}

		T getValue() {
			_stateMutex.lock();
			T ret = _value;
			_stateMutex.unlock();

			return ret;
		}

		void addValueChangedListener(Command* command) {
			_valueChangedEventCommands.push_back(command);
		}

		void removeValueChangedEventCommand(Command* command) {
			std::vector<Command*>::iterator it = std::find(_valueChangedEventCommands.begin(), _valueChangedEventCommands.end(), command);

			if (it != _valueChangedEventCommands.end()) {
				_valueChangedEventCommands.erase(std::remove(_valueChangedEventCommands.begin(), _valueChangedEventCommands.end(), command),
					_valueChangedEventCommands.end());
			}
		}

	private:
		T _value;
		Mutex _stateMutex;
		std::vector<Command*> _valueChangedEventCommands;

		void _onValueChanged() {
			for (Command* command : _valueChangedEventCommands) {
				command->execute((CommandArgs)&_value);
			}
		}
};

#endif