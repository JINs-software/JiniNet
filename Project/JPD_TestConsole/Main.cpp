#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;
#include "JPDComiler.h"
#include "JNetPool.h"

#include "JMemoryHistory.h"

struct stTemp {
	int a;
	char b;
	short c;
};


///////////////////////////
// JMemoryHistory 테스트
///////////////////////////
//#define new new(X)	new(__FILE__, __LINE) X;
// -> new(char[100]); 형식으로 사용
//#define new		new(__FILE__, __LINE__)
// -> new char[100]; 형식으로 사용

// JMemoryHistory 테스트 용 클래스
class Base {
public:
	Base() {
		cout << "Test 클래스 생성자" << endl;
	}
	int a;
	int b;
	int c;
};
class A : public Base {
public:
	A() {
		cout << "A 생성자" << endl;
	}
};
////////////////////////////////////


int main() {
	////////////////////////
	// JPD 생성 테스트
	////////////////////////
	//JPDComiler jpdcomp;
	//jpdcomp.SetPacektIDType("BYTE");
	//jpdcomp.SetPacektSizeType("BYTE");
	//jpdcomp.SetOneway();
	//string filePath = ".\\json\\OnewayFightGame.json";
	//string output = "D:\\0. GameSeverStudy\\1. Project\\MMO_Figther\\MMO_Figther\\RPC\\";
	////string output = ".\\json\\";
	//
	//jpdcomp.CompileJPD(filePath, output);


	////////////////////////
	// JiniPool 테스트
	////////////////////////
	//JiniPool pool(sizeof(stTemp), 10);
	//
	//vector<stTemp*> vec(10);
	//
	//for (int i = 0; i < 10; i++) {
	//	vec[i] = reinterpret_cast<stTemp*>(pool.AllocMem());
	//	if (vec[i] != NULL) {
	//		vec[i]->a = i;
	//		vec[i]->b = i;
	//		vec[i]->c = i;
	//	}
	//}
	//
	//pool.ReturnMem(reinterpret_cast<BYTE*>(vec[3]));
	//pool.ReturnMem(reinterpret_cast<BYTE*>(vec[5]));



	///////////////////////////
	// JMemoryHistory 테스트
	///////////////////////////
	//A* p1 = new A[10];
}