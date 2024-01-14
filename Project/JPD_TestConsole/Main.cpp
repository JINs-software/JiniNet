#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;
#include "JPDComiler.h"

#include "JNetPool.h"

struct stTemp {
	int a;
	char b;
	short c;
};

int main() {
	// JPD »ý¼º
	//JPDComiler jpdcomp;
	//jpdcomp.SetPacektIDType("BYTE");
	//jpdcomp.SetPacektSizeType("BYTE");
	//jpdcomp.SetOneway();
	//string filePath = ".\\json\\OnewayFightGame.json";
	//string output = "D:\\0. GameSeverStudy\\1. Project\\MMO_Figther\\MMO_Figther\\RPC\\";
	////string output = ".\\json\\";
	//
	//jpdcomp.CompileJPD(filePath, output);

	JiniPool pool(sizeof(stTemp), 10);
	
	vector<stTemp*> vec(10);

	for (int i = 0; i < 10; i++) {
		vec[i] = reinterpret_cast<stTemp*>(pool.AllocMem());
		if (vec[i] != NULL) {
			vec[i]->a = i;
			vec[i]->b = i;
			vec[i]->c = i;
		}
	}

	pool.ReturnMem(reinterpret_cast<BYTE*>(vec[3]));
	pool.ReturnMem(reinterpret_cast<BYTE*>(vec[5]));
}