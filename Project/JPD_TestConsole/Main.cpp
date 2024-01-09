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
	string filePath = ".\\json\\OnewayFightGame.json";
	string output = "D:\\0. GameSeverStudy\\1. Project\\MMO_Figther\\MMO_Figther\\RPC\\";
	//string output = ".\\json\\";

	jpdcomp.CompileJPD(filePath, output);

}