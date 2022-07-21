#ifndef _STEERING_UTIL_H_
#define _STEERING_UTIL_H_

namespace util {
	typedef void* CommandArgs;
	class Command {
		public:
			virtual ~Command() { }
			virtual void execute(CommandArgs args) = 0;
	};

	template <class T>
	class FunctionalCallback : public Command {
		public:
			FunctionalCallback(void (*callback)(const T)) : _callback(callback) { }
			~FunctionalCallback() { }
			virtual void execute(CommandArgs args) {
				_callback(*((T*)args));
			}

		private:
			void (*_callback)(const T);
	};

	template <class T, class R>
	class Delegate : public Command {
		public:
			Delegate(T* owner, void (T::*callback)(const R)) : _owner(owner), _callback(callback) { }
			~Delegate() { }
			void execute(CommandArgs args) override {
				(_owner->*_callback)(*((R*)args));
			}
		private:
			T* _owner;
			void (T::*_callback)(const R);
	};
}

#endif