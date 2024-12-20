/*
 * Fmutex.h
 *
 *  Created on: 15.8.2017
 *      Author: krl
 */

#ifndef FMUTEX_H_
#define FMUTEX_H_

#include "FreeRTOS.h"
#include "semphr.h"
#include <mutex>

class Fmutex {
public:
	Fmutex();
	~Fmutex();
	void lock();
	void unlock();
private:
	SemaphoreHandle_t mutex;
};

#endif /* FMUTEX_H_ */
