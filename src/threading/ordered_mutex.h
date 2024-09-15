// Minetest
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <cstdint>
#include <condition_variable>
#include "mutex_auto_lock.h"

/*
	Fair mutex based on ticketing approach.
	Satisfies `BasicLockable` requirement.
*/
class ordered_mutex {
public:
	ordered_mutex() : next_ticket(0), counter(0) {}

	void lock()
	{
		MutexAutoLock autolock(cv_lock);
		const auto ticket = next_ticket++;
		cv.wait(autolock, [&] { return counter == ticket; });
	}

	void unlock()
	{
		MutexAutoLock autolock(cv_lock);
		counter++;
		cv.notify_all();
	}

private:
	std::condition_variable cv;
	std::mutex cv_lock;
	uint_fast32_t next_ticket, counter;
};

using OrderedMutexAutoLock = std::unique_lock<ordered_mutex>;
