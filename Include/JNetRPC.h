#pragma once

#define HDR_UNIQUE_NUM_TYPE				unsigned char
#define HDR_MSG_ID_TYPE					unsigned short
#define HDR_MSG_LEN_TYPE				unsigned short

struct stMSG_HDR {
	HDR_UNIQUE_NUM_TYPE		uniqueNum;
	HDR_MSG_ID_TYPE			msgID;
	HDR_MSG_LEN_TYPE		msgLen;
};

#define UNIQUE_PACK_NUM 0x77