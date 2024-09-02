#pragma once
#include <string>
#include <vector>
#include "../rapidjson/document.h"
#include "../rapidjson/writer.h"
#include "../rapidjson/stringbuffer.h"

struct stJParam {
	std::string type;
	std::string name;
};
struct stJPDef {
	std::string name;
	std::string dir;
	std::vector<stJParam> params;
	uint16_t enumAdd = 0;
};
struct stJPD {
	std::string jpdNameSpace;
	uint16_t enumeration;
	std::vector<stJPDef> jps;
};
struct stOnewayHdr {
	uint32_t	msgHdrSize;
	uint32_t	msgLenOffset;
	std::string msgLenType;
	uint32_t	msgIdOffset;
	std::string msgIdType;
};

class JPDComiler
{
private:
	rapidjson::Document jpdDoc;
	std::vector<stJPD> jpdVec;
	std::string packetSizeType = "uint16_t";
	std::string packetIDType = "uint16_t";

	bool oneWay = false;
	stOnewayHdr onewayHdr;

public:
	void CompileJPD(std::string filePath, std::string outputPath);
	inline void SetPacektSizeType(const std::string& type) { packetSizeType = type; }
	inline void SetPacektIDType(const std::string& type) { packetIDType = type; }
	inline void SetOneway() { oneWay = true; }
	
private:
	void loadJPDFile(std::string filePath);
	void parseJPD();
	void makeModule(std::string outputPath);

	void makeProxy(std::ofstream& pxyHdr, std::ofstream& pxyCpp, const stJPD& jpd, std::string dir);
	void makeStub(std::ofstream& stbHdr, std::ofstream& stbCpp, const stJPD& jpd, std::string dir);
	void makeProxy(std::ofstream& pxyHdr, std::ofstream& pxyCpp, const stJPD& jpd);
	void makeStub(std::ofstream& stbHdr, std::ofstream& stbCpp, const stJPD& jpd);
	void makeComm(std::ofstream& commHdr, std::ofstream& commCpp, const stJPD& jpd, std::string dir);
	void makeComm(std::ofstream& commHdr, std::ofstream& commCpp, const stJPD& jpd);

	void sizeofStr(std::ofstream& pxyCpp, const std::vector<stJParam>& params);
	void leftShiftStr(std::ofstream& pxyCpp, const std::string& pname, const std::vector<stJParam>& params);
};

