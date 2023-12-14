#pragma once
#include <string>
#include <vector>
#include "../rapidjson/document.h"
#include "../rapidjson/writer.h"
#include "../rapidjson/stringbuffer.h"
//enum enJPDir {
//	S2C,	// 0
//	C2S		// 1
//};

struct stJParam {
	std::string type;
	std::string name;
};
struct stJPDef {
	std::string name;
	std::string dir;
	std::vector<stJParam> params;
};
struct stJPD {
	std::string jpdNameSpace;
	uint16_t enumeration;
	std::vector<stJPDef> jps;
};

class JPDComiler
{
private:
	rapidjson::Document jpdDoc;
	std::vector<stJPD> jpdVec;
	std::string packetSizeType = "uint16_t";
	std::string packetIDType = "uint16_t";
	bool oneWay = false;

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

