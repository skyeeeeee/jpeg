//Main.cpp
#include <iostream>
#include "jenc.h"

using namespace std;

int main()
{
	string sBmpName;
	cout << "Please input bmp filename:\n";
	cin >> sBmpName;
	string outFile = sBmpName.substr(0,sBmpName.find_last_of('.'));
	outFile = outFile + ".jpg";
	
	JEnc enc;
	enc.Invoke(sBmpName, outFile,90);
	cout << "Out:" << outFile << endl;
	cout <<"finish"<<endl;
	getchar();
	
	return 0;
}

/* ÊäÈëmain²ÎÊý
int main(int argc, char* argv[])
{
 if (argc <= 1)
 {
  cout << "please input bmp filename." << endl;
  return 0;
 }

// string fileName = string("E:\\Work\\VC\\Temp\\jpeg\\jpeg_mine\\Debug\\duanwu.bmp");
 string fileName = string(argv[0]);
 string outFile = fileName.substr(0,fileName.find_last_of('.'));
 outFile = outFile + ".jpg";

 JEnc enc;
 enc.Invoke(fileName, outFile,90);
 cout << "Out:" << outFile << endl;
 cout <<"finish"<<endl;
 getchar();

 return 0;
}
*/
