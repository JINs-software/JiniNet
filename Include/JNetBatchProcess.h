#pragma once

class JNetBatchProcess {
public:
	virtual void BatchProcess() {}
	virtual void BatchProcess(uint16 calibration) {}

	virtual void BatchConsoleLog() {}
	virtual void BatchValidCheck() {}
};