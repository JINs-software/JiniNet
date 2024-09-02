#pragma once

class JNetBatchProcess {
public:
	virtual void BatchProcess(uint16 frameDelta) {}
	virtual void BatchConsoleLog() {}
	virtual void BatchValidCheck() {}
};