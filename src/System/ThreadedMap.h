#ifndef _THREADED_MAP_H_
#define _THREADED_MAP_H_

#include <unordered_map>
#include <Mutex.h>

template <class K, class V>
class ThreadedMap {
	public:
		~ThreadedMap() { }

		V& operator[](const K& k) {
			_stateMutex.lock();
			V& ret = _map[k];
			_stateMutex.unlock();

			return ret;
		}

		typename std::unordered_map<K, V>::iterator find(K key) {
			_stateMutex.lock();
			auto ret = _map.find(key);
			_stateMutex.unlock();

			return ret;
		}

		void erase(K key) {
			_stateMutex.lock();
			_map.erase(key);
			_stateMutex.unlock();
		}

		typename std::unordered_map<K, V>::iterator begin() {
			_stateMutex.lock();
			auto ret = _map.begin();
			_stateMutex.unlock();
			
			return ret;
		}

		typename std::unordered_map<K, V>::iterator end() {
			_stateMutex.lock();
			auto ret = _map.end();
			_stateMutex.unlock();
			
			return ret;
		}

		bool containsKey(K key) {
			_stateMutex.lock();
			bool ret = _map.find(key) != _map.end();
			_stateMutex.unlock();

			return ret;
		}

	private:
		std::unordered_map<K, V> _map;
		Mutex _stateMutex;
};

#endif