#include "JNetStub.h"

RpcID* JNetStub::GetRpcList() { return nullptr; }
int JNetStub::GetRpcListCount() { return 0; }

void JNetStub::ProcessReceivedMessage(HostID remote, JBuffer& jbuff) { return; }