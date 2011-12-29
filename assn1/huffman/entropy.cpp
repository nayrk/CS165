#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>

using namespace std;

//Entropy of bytes in a file

int main(){
	ifstream file;
	file.open("compressedFile.txt");
	if(!file.is_open()) cout << "File not found";

	//256 for ascii values initialized to 0
	vector<int> probs(256,0);

	int c;
	int count = 0;
	//Read file and parse token
	while(file.good()){
		count++;
		c = file.get();
		if(c < 0) continue;
		probs[c] = probs[c] + 1;
	}
	file.close();

	double entropy = 0.0;
	//iterating through each ascii value
	for(int i = 0; i < probs.size(); i++){
		double p_i = (double)probs[i] / (double)count;
		if(p_i > 0.0){
			//Using formula E[x] to calculate entropy
			//Formula provided from lab
			//Using math.h class to take log
			//Change of base to formula to handle base 2
			entropy -= p_i * (log(p_i) / log(2));
		}
	}
	cout << "Entropy value: " << entropy << endl;
	return 0;
}
