#include <iostream>
#include <fstream>
#include <string>
using namespace std;

int main() {
	string name;
	string path;
	cin >> path; // file path
	cin >> name; // variables name

	ofstream writefp("resource.c", ios::out | ios::app | ios::binary);
	if (writefp.is_open()) {
		ifstream readfp(path);
		if (readfp.is_open()) {
			cout << path << " convert cpp variables!!" << endl;
			writefp << "unsigned char " << name << "[] = {";
			int i = 0;
			for (i = 0; readfp.eof() == false; i++) {
				unsigned char byte = readfp.get();
				if (i % 10 == 0) writefp << endl << "\t";
				writefp << "0x" << hex << (int)byte << ",\t";
			}
			cout << "converted end!!" << endl;
			writefp << "};" << endl;
			writefp << "int " << name << "_len = 0x" << i - 1 << ";" << endl << endl;
			readfp.close();
		} else {
			cout << path << " is not open!!" << endl;
		}
		writefp.close();
	}
	return 0;
}

