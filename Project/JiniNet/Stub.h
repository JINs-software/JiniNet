#pragma once
#include "JNetStub.h"

class Stub : public JNetStub {
	bool ProcessReceivedMessage(HostID id, JBuffer& recvBuff) override {
		stMSG_HDR hdr;
		if (recvBuff.GetUseSize() < sizeof(stMSG_HDR)) {
			return false;
		}
		recvBuff.Peek(&hdr);
		if (hdr.uniqueNum != UNIQUE_PACK_NUM || sizeof(stMSG_HDR) + hdr.msgLen > recvBuff.GetUseSize()) {
			return false;
		}

		recvBuff.DirectMoveDequeueOffset(sizeof(stMSG_HDR));

		switch (hdr.msgID) {
		//case MOVE:
		//{
		//	break;
		//}
		//case ATTACK: 
		//{
		//
		//	break;
		//}
		// ..
		}
	}
};