#pragma once
#include "JNetProxy.h"

#define MAXIMUM_MSG_LEN 100

class Proxy : public JNetProxy {
	bool Move(HostID remote, int a, int b, int c) {
		UINT msgLen = sizeof(a) + sizeof(b) + sizeof(c);
		JBuffer jbuff(msgLen);
		jbuff << a << b << c;
		Send(remote, jbuff);
	}
	//..
	// ..
};