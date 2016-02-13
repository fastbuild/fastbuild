// WorkerBrokerage - Manage worker discovery
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_WORKERBROKERAGE_H
#define FBUILD_WORKERBROKERAGE_H

// Includes
//------------------------------------------------------------------------------
#include "Core/Strings/AString.h"
#include "Core/Time/Timer.h"

// Forward Declarations
//------------------------------------------------------------------------------

// WorkerBrokerage
//------------------------------------------------------------------------------
class WorkerBrokerage
{
public:
	WorkerBrokerage();
	~WorkerBrokerage();

	// client interface
	void FindWorkers( Array< AString > & workerList );

	// server interface
	void SetAvailability( bool available );
private:
	void Init();

	AString				m_BrokerageRoot;
	bool				m_Availability;
	bool				m_Initialized;
    AString             m_HostName;
    AString             m_BrokerageFilePath;
    Timer               m_TimerLastUpdate;      // Throttle network access
};

//------------------------------------------------------------------------------
#endif // FBUILD_WORKERBROKERAGE_H 
