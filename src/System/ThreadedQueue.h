#ifndef _THREADED_QUEUE_H_
#define _THREADED_QUEUE_H_

#include <queue>
#include <Mutex.h>

template <class T>
class ThreadedQueue {
	public:
		ThreadedQueue() { }
		~ThreadedQueue() { }
		
		void push(T elem) {
			_stateMutex.lock();
			_queue.push(elem);
			_stateMutex.unlock();
		}

		const T& front() {
			_stateMutex.lock();
			T& ret = _queue.front();
			_stateMutex.unlock();

			return ret;
		}

		void pop() {
			_stateMutex.lock();
			_queue.pop();
			_stateMutex.unlock();
		}

		bool empty() {
			_stateMutex.lock();
			bool ret = _queue.empty();
			_stateMutex.unlock();

			return ret;
		}

	private:
		std::queue<T> _queue;
		Mutex _stateMutex;
};

#endif