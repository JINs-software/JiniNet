#include "JNetStub.h"

RpcID* JNetStub::GetRpcList() { return nullptr; }
int JNetStub::GetRpcListCount() { return 0; }

bool JNetStub::ProcessReceivedMessage(HostID remote, JBuffer& jbuff) { return false; }