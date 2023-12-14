#include "JPDComiler.h"
#include <iostream>
#include <fstream>
using namespace std;

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
using namespace rapidjson;

void JPDComiler::CompileJPD(std::string filePath, std::string outputPath)
{
	loadJPDFile(filePath);
	parseJPD();
	makeModule(outputPath);
}

void JPDComiler::loadJPDFile(std::string filePath)
{
	ifstream file(filePath);
	if (!file.is_open()) {
		// TO DO: 俊矾 贸府
		cerr << "Error: Could not open file, errno: " << errno << endl;
		perror("Error details");
		return;
	}

	string line, text;
	while (getline(file, line)) {
		text += line + "\n";
	}
	const char* pJson = text.c_str();
	
	jpdDoc.Parse(pJson);
}

void JPDComiler::parseJPD()
{
	Value& jpdArray = jpdDoc["JPD"];
	for (SizeType i = 0; i < jpdArray.Size(); i++) {
		Value& jpdObject = jpdArray[i];
		stJPD jpd;
		jpd.jpdNameSpace = jpdObject["Namespace"].GetString();
		jpd.enumeration = jpdObject["Enum"].GetUint();

		Value& jpdefArray = jpdObject["Def"];
		for (SizeType j = 0; j < jpdefArray.Size(); j++) {
			Value& jpdefObject = jpdefArray[j];
			stJPDef jpdef;
			jpdef.name = jpdefObject["Name"].GetString();
			jpdef.dir = jpdefObject["Direction"].GetString();

			Value& jparams = jpdefObject["Parameter"];
			for (SizeType k = 0; k < jparams.Size(); k++) {
				Value& jparamObject = jparams[k];
				stJParam jparam;
				jparam.type = jparamObject["type"].GetString();
				jparam.name = jparamObject["name"].GetString();

				jpdef.params.push_back(jparam);
			}
			jpd.jps.push_back(jpdef);
		}
		jpdVec.push_back(jpd);
	}
}

void JPDComiler::makeModule(std::string outputPath)
{
	for (stJPD& jpd : jpdVec) {
		ofstream proxyHdr;
		ofstream proxyCpp;
		ofstream stubHdr;
		ofstream stubCpp;
		ofstream commHdr;
		ofstream commCpp;
		if (jpd.jpdNameSpace.length() == 0) {
			proxyHdr.open(outputPath + "\Proxy.h", ios::trunc);
			proxyCpp.open(outputPath + "\Proxy.cpp", ios::trunc);
			stubHdr.open(outputPath + "\Stub.h", ios::trunc);
			stubCpp.open(outputPath + "\Stub.cpp", ios::trunc);
			commHdr.open(outputPath + "\Common.h", ios::trunc);
			commCpp.open(outputPath + "\Common.cpp", ios::trunc);
		}
		else {
			proxyHdr.open(outputPath + "\Proxy_" + jpd.jpdNameSpace + ".h", ios::trunc);
			proxyCpp.open(outputPath + "\Proxy_" + jpd.jpdNameSpace + ".cpp", ios::trunc);
			stubHdr.open(outputPath + "\Stub_" + jpd.jpdNameSpace + ".h", ios::trunc);
			stubCpp.open(outputPath + "\Stub_" + jpd.jpdNameSpace + ".cpp", ios::trunc);
			commHdr.open(outputPath + "\Common_" + jpd.jpdNameSpace + ".h", ios::trunc);
			commCpp.open(outputPath + "\Common_" + jpd.jpdNameSpace + ".cpp", ios::trunc);
		}
		
		if (!proxyHdr.is_open() || !proxyCpp.is_open() || !stubHdr.is_open() || !stubCpp.is_open() || !commHdr.is_open() || !commCpp.is_open()) {
			// TO DO: 俊矾 贸府
			return;
		}

		//for (stJPD& jpd : jpdVec) {
		//	makeProxy(proxyHdr, proxyCpp, jpd);
		//	makeStub(stubHdr, stubCpp, jpd);
		//}
		makeProxy(proxyHdr, proxyCpp, jpd);
		makeStub(stubHdr, stubCpp, jpd);
		makeComm(commHdr, commCpp, jpd);
	}
}

void JPDComiler::makeProxy(std::ofstream& pxyHdr, std::ofstream& pxyCpp, const stJPD& jpd, string dir)
{
	pxyHdr << "namespace " << jpd.jpdNameSpace << dir << " {" << endl << endl;
	pxyCpp << "namespace " << jpd.jpdNameSpace << dir << " {" << endl << endl;

	pxyHdr << "\t" << "class Proxy : public JNetProxy" << endl;
	pxyHdr << "\t" << "{" << endl;
	pxyHdr << "\t" << "public: " << endl;
	for (const stJPDef& jpdef : jpd.jps) {
		if (jpdef.dir.compare(dir) == 0) {
			pxyHdr << "\t\t" << "virtual bool " << jpdef.name << "(";
			pxyCpp << "\t" << "bool Proxy::" << jpdef.name << "(";
			pxyHdr << "HostID remote";
			pxyCpp << "HostID remote";
			for (const stJParam& jparam : jpdef.params) {
				pxyHdr << ", " << jparam.type << " " << jparam.name;
				pxyCpp << ", " << jparam.type << " " << jparam.name;
			}
			pxyHdr << ");" << endl;
			pxyCpp << ") {" << endl;
			pxyCpp << "\t\t" << "uint32_t msgLen = ";
			sizeofStr(pxyCpp, jpdef.params);
			pxyCpp << endl;
			pxyCpp << "\t\t" << "uint32_t hdrLen = sizeof(uniqueNum) + sizeof(" << packetSizeType << ") + sizeof(" << packetIDType << ");" << endl;
			pxyCpp << "\t\t" << "JBuffer buff(msgLen + hdrLen);" << endl;
			leftShiftStr(pxyCpp, jpdef.name, jpdef.params);
			pxyCpp << "\t\tSend(remote, buff);" << endl << endl;
			pxyCpp << "\t\t" << "return true;" << endl;
			pxyCpp << "\t}" << endl;
		}
	}
	pxyHdr << endl;
	pxyHdr << "\t\t" << "RpcID* GetRpcList() override { return gRpcList; }" << endl;
	pxyHdr << "\t\t" << "int GetRpcListCount() override { return gRpcListCount; }" << endl;
	pxyHdr << "\t};" << endl << endl;
	pxyHdr << "}" << endl << endl;
	pxyCpp << "}" << endl << endl;
}

void JPDComiler::makeStub(std::ofstream& stbHdr, std::ofstream& stbCpp, const stJPD& jpd, string dir)
{
	stbHdr << "namespace " << jpd.jpdNameSpace << dir << " {" << endl << endl;
	stbCpp << "namespace " << jpd.jpdNameSpace << dir << " {" << endl << endl;
	stbHdr << "\t" << "class Stub : public JNetStub" << endl;
	stbHdr << "\t" << "{" << endl;
	stbHdr << "\t" << "public: " << endl;
	stbCpp << "\t" << "bool Stub::ProcessReceivedMessage(HostID remote, JBuffer& jbuff) {" << endl;
	stbCpp << "\t\t" << "stMSG_HDR hdr;" << endl;
	stbCpp << "\t\t" << "jbuff.Dequeue(reinterpret_cast<BYTE*>(&hdr), sizeof(stMSG_HDR));" << endl;
	stbCpp << "\t\t" << "if(hdr.uniqueNum != uniqueNum) {" << endl;
	stbCpp << "\t\t\t" << "return false;" << endl;
	stbCpp << "\t\t" << "}" << endl << endl;
	stbCpp << "\t\t" << "switch(hdr.msgID) {" << endl;
	for (const stJPDef& jpdef : jpd.jps) {
		if (jpdef.dir.compare(dir) == 0) {
			stbHdr << "\t\t" << "virtual bool ";
			string fdef = jpdef.name + "(";
			string fcall = jpdef.name + "(";
			fdef += "HostID remote";
			fcall += "remote";

			stbCpp << "\t\t" << "case " << "RPC_" << jpdef.name << ":" << endl;
			stbCpp << "\t\t" << "{" << endl;

			for (const stJParam& jparam : jpdef.params) {
				fdef += ", " + jparam.type + " " + jparam.name;
				fcall += ", " + jparam.name;
				stbCpp << "\t\t\t" << jparam.type << " " << jparam.name << ";" << endl;
				stbCpp << "\t\t\t" << "jbuff >> " << jparam.name << ";" << endl;
			}
			fdef += ")";
			fcall += ")";
			stbHdr << fdef;
			stbHdr << " { return false; }" << endl;

			stbHdr << "#define JPDEC_" << jpd.jpdNameSpace << dir << "_" << jpdef.name << " bool " << fdef << endl;
			stbHdr << "#define JPDEF_" << jpd.jpdNameSpace << dir << "_" << jpdef.name << "(DerivedClass) " << "bool DerivedClass::" << fdef << endl;

			stbCpp << "\t\t\t" << fcall << ";" << endl;
			stbCpp << "\t\t" << "}" << endl;
			stbCpp << "\t\t" << "break;" << endl;
		}
	}
	stbHdr << endl;
	stbHdr << "\t\t" << "RpcID* GetRpcList() override { return gRpcList; }" << endl;
	stbHdr << "\t\t" << "int GetRpcListCount() override { return gRpcListCount; }" << endl << endl;
	stbHdr << "\t\t" << "bool ProcessReceivedMessage(HostID remote, JBuffer& jbuff) override;" << endl;
	stbHdr << "\t};" << endl << endl;
	stbHdr << "}" << endl;
	stbCpp << "\t\t" << "}" << endl;
	stbCpp << "\t" << "}" << endl;
	stbCpp << "}" << endl;
}

void JPDComiler::makeProxy(ofstream& pxyHdr, ofstream& pxyCpp, const stJPD& jpd)
{
	pxyHdr << "#pragma once" << endl << endl;
	pxyHdr << "#include \"Common_" << jpd.jpdNameSpace << ".h\"" << endl << endl;
	pxyCpp << "#include \"Proxy_" << jpd.jpdNameSpace << ".h\"" << endl << endl;

	bool s2cFlag = false;
	bool c2sFlag = false;
	for (const stJPDef& jpdef : jpd.jps) {
		if (jpdef.dir.compare("S2C") == 0) s2cFlag = true;
		else if (jpdef.dir.compare("C2S") == 0) c2sFlag = true;
	}
	if(s2cFlag) makeProxy(pxyHdr, pxyCpp, jpd, "S2C");
	if(c2sFlag) makeProxy(pxyHdr, pxyCpp, jpd, "C2S");
}

void JPDComiler::makeStub(ofstream& stbHdr, ofstream& stbCpp, const stJPD& jpd)
{
	stbHdr << "#pragma once" << endl << endl;
	stbHdr << "#include \"Common_" << jpd.jpdNameSpace << ".h\"" << endl << endl;
	stbCpp << "#include \"Stub_" << jpd.jpdNameSpace << ".h\"" << endl << endl;

	bool s2cFlag = false;
	bool c2sFlag = false;
	for (const stJPDef& jpdef : jpd.jps) {
		if (jpdef.dir.compare("S2C") == 0) s2cFlag = true;
		else if (jpdef.dir.compare("C2S") == 0) c2sFlag = true;
	}
	if (s2cFlag) makeStub(stbHdr, stbCpp, jpd, "S2C");
	if (c2sFlag) makeStub(stbHdr, stbCpp, jpd, "C2S");
}

void JPDComiler::makeComm(std::ofstream& commHdr, std::ofstream& commCpp, const stJPD& jpd, std::string dir)
{
	commHdr << "namespace " << jpd.jpdNameSpace << dir << " {" << endl << endl;
	commCpp << "namespace " << jpd.jpdNameSpace << dir << " {" << endl << endl;

	commCpp << "\tRpcID gRpcList[] = {" << endl;

	uint16_t jpdEnum = jpd.enumeration;
	int cnt = 0;
	for (int i = 0; i < jpd.jps.size(); i++) {
		if (jpd.jps[i].dir.compare(dir) == 0) {
			commHdr << "\tstatic const RpcID RPC_" << jpd.jps[i].name << " = " << jpdEnum + i << ";" << endl;
			commCpp << "\t\tRPC_" << jpd.jps[i].name << "," << endl;
			cnt++;
		}
	}

	commCpp << "\t};" << endl << endl;
	commCpp << "\tint gRpcListCount = " << cnt << ";" << endl << endl;
	commHdr << endl;
	commHdr << "\textern RpcID gRpcList[];" << endl;
	commHdr << "\textern int gRpcListCount;" << endl << endl;

	commHdr << "}" << endl << endl;
	commCpp << "}" << endl << endl;
}

void JPDComiler::makeComm(std::ofstream& commHdr, std::ofstream& commCpp, const stJPD& jpd)
{
	commHdr << "#pragma once" << endl << endl;
	commCpp << "#include \"Common_" << jpd.jpdNameSpace << ".h\"" << endl << endl;

	bool s2cFlag = false;
	bool c2sFlag = false;
	for (const stJPDef& jpdef : jpd.jps) {
		if (jpdef.dir.compare("S2C") == 0) s2cFlag = true;
		else if (jpdef.dir.compare("C2S") == 0) c2sFlag = true;
	}
	if (s2cFlag) makeComm(commHdr, commCpp, jpd, "S2C");
	if (c2sFlag) makeComm(commHdr, commCpp, jpd, "C2S");
}

void JPDComiler::sizeofStr(std::ofstream& pxyCpp, const std::vector<stJParam>& params)
{
	for (int i = 0; i < params.size(); i++) {
		pxyCpp << "sizeof(" << params[i].name << ")";
		if (i == params.size() - 1) {
			pxyCpp << ";";
		}
		else {
			pxyCpp << " + ";
		}
	}
}

void JPDComiler::leftShiftStr(std::ofstream& pxyCpp, const std::string& pname, const std::vector<stJParam>& params)
{
	pxyCpp << "\t\tbuff << uniqueNum;" << endl;
	pxyCpp << "\t\tbuff << msgLen;" << endl;
	pxyCpp << "\t\tbuff << RPC_" << pname << ";" << endl;
	for (int i = 0; i < params.size(); i++) {
		pxyCpp << "\t\tbuff << " << params[i].name << ";" << endl;
	}
}

