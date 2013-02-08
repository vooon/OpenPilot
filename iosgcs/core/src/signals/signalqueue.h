/*
 * ( c ) 2013 The OpenPilot
 */

#ifndef SIGNALQUEUE_H
#define SIGNALQUEUE_H

#include "signals_impl.h"

class CL_SignalQueue
{
public:
	CL_SignalQueue(CL_SharedPtr<CL_SignalQueue_Impl> queue) : m_impl(queue) { }

	CL_SharedPtr<CL_SignalQueue_Impl> m_impl;

	void invoke() {
		if (m_impl != 0)
			m_impl->invoke();
	}
};

#endif // SIGNALQUEUE_H
