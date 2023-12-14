#include <iostream>
#include <fstream>
#include <string>
using namespace std;
#include "JPDComiler.h"

int main() {
	JPDComiler jpdcomp;
	jpdcomp.SetPacektIDType("BYTE");
	jpdcomp.SetPacektSizeType("BYTE");
	jpdcomp.SetOneway();
	string filePath = ".\\json\\RPCFightGame.json";
	string output = "D:\\0. GameSeverStudy\\0. JiniNet\\RPCFightGame\\RPCFightGame\\RPC\\";

	jpdcomp.CompileJPD(filePath, output);

}