#include <iostream>
#include <fstream>

using namespace std;

int main(){
	ifstream file("rotateN.rb");
	if(!file.is_open()){cout << "File Not found"; return 0; }
	ofstream out("comp");

	short c;
	int x_i = 11235;
	while(file.good()){
		int c1 = file.get();	
		int c2 = file.get();	
		c = c1 << 8; 
		c = c | c2;
		c = c^x_i;
		x_i = (377*x_i + 2971) % 64249;
		out << ((char)(c >> 8)) << (char)c;
	}

	file.close();
	out.close();

	ifstream in("comp");
	if(!in.is_open()){cout << "File Not found"; return 0; }

	ofstream out2("uncomp");
	x_i = 11235;
	while(in.good()){
		int c1;
		int c2;
		short c;	
		c1 = in.get();
		c2 = in.get();

		//printf("c1 = %X ", c1);
		//printf("c2 = %X ", c2);

		c = c1 << 8; 
		c = c | c2;
		int o = x_i^c;
		x_i = (377*x_i + 2971) % 64249;
		out2 << ((char)(o >> 8)) << (char)o;
	}
	in.close();
	out2.close();

	/*Finding Modulus M
	  /
	  int x = 104339511;
	  int value;
	  for(int i = 41578; i < 65536; i++){
	  value = (x % i);	
	  if(value == 0){
	  cout << i << endl;
	  break;
	  }
	  }
	 */

	/*
	 * Finding a
	 long int value;
	 for(long int i = 1; i < 100000000000; i++){
	 value = (24262 * i) % 64249;
	 if (value == 23416)
	 {
	 cout << i << endl;
	 break;
	 }
	 }
	 */

	/*
	 * Finding b
	 }
	 long int value;
	 for(long int i = 1; i < 100000000000; i++){
	 value = ((744*377) + i) % 64249;
	 if (value == 26463)
	 {
	 cout << i << endl;
	 break;
	 }
	 }
	 */


return 0;
}
