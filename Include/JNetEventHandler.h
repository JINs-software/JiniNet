#pragma once
class JNetEventHandler
{
public:
	virtual void OnError() {}
	virtual void OnWarning() {}
	virtual void OnException() {}
	virtual void OnNoOverrideStub() {}
};

