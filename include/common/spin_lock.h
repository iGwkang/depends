//
// Created by Gwkang on 2021/12/05.
//

#pragma once

#include <atomic>

class spin_lock
{
public:
	spin_lock() = default;
	spin_lock(const spin_lock &) = delete;
	spin_lock(spin_lock &&) = delete;
	spin_lock& operator=(const spin_lock &) = delete;

	void lock()
	{
		uint8_t backoff = 1;

		while (flag.test_and_set(std::memory_order_acquire))
		{
			for (uint8_t i = 0; i < backoff; i++)
				std::this_thread::yield();
			if (backoff < max_back_off)
				backoff <<= 1;
		}
	}

	bool try_lock()
	{
		return !flag.test_and_set(std::memory_order_acquire);
	}

	void unlock()
	{
		flag.clear(std::memory_order_release);
	}

private:
	constexpr static uint8_t max_back_off = 16;

	std::atomic_flag flag = ATOMIC_FLAG_INIT;
};
