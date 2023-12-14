#pragma once
#include "JNetEventHandler.h"

class JNetClientEventHandler : public JNetEventHandler
{
public:
	virtual void OnReadyToConnect() {}
	virtual void OnAcceptComplete() {}
};

