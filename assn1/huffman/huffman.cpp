#include <iostream>
#include <fstream>
#include <vector>
#include <assert.h>
#include <queue>

using namespace std;


/*
Algorithm From: Wikipedia Entry
Also From: //http://www.siggraph.org/education/materials/HyperGraph/video/mpeg/mpegfaq/huffman_tutorial.html
*/

//stream class will write to the file when 8 bits are "full" in the char
class stream{
	public:
		char buffer; //8 bits for 1 byte
		int count; //count the bits to reset and place in position
		ostream & out; //stream reference to write to output file

		//ostream forces this type of initializaton since it has no assignment operator
		//didnt know took a while to figure out from compiler
		stream(ostream & file):out(file){
			buffer = 0;
			count = 0;
		}

		void bit(int i){
			//Write to file when it hits 8 bits
			//Reset everything
			if(count ==	8){
				out << buffer;	
				buffer = 0;
				count = 0;
			}

			//Continue to shift each bit down to its correct place
			//and then you OR it to append it
			if(i == 1){
				buffer = buffer | (1 << (7 - count));
			}else{

				buffer = buffer | (0 << (7 - count));
			}
			//Increment counter
			count++;
		}
};
//Class Node that are "leaves"
//
class Node{
	public:
		int dir; //direction if its a 0 or 1, this is used later to construct the path
		char symbol; //char symbol (ascii value)
		int freq; //frequency total
		vector<int> encodedPath; //encoded path to this char
		Node * left; //0 child
		Node * right; //1 child
		Node * parent; //Parent

		//Constructor
		Node(char s,int f,Node * l,Node * r,Node * p){
			symbol = s;
			freq = f;
			left = l;
			right = r;
			parent = p;
			dir = 0;
		}

		//Comparison overload by freq
		bool operator < (const Node & other){
			if(freq != other.freq){
				return freq > other.freq;
			}
		
			//If weights are equal, compare char # value
			return symbol < other.symbol;
		}
};

//Compare to be used in STL pQueue container
//Example from 
//http://comsci.liu.edu/~jrodriguez/cs631sp08/c++priorityqueue.html
class compare{
	public:
		//This will call the overloaded compartor from Node class
		//() acts as a <
		bool operator()(Node *& left, Node *& right){
			return *left < *right;
		}
};

class HuffmanTree{
		public:
			//Pointer to the top of the tree
			Node* root;
			//Leaves in the tree with freq > 0
			vector<Node*> leaves;

			vector<int> path(Node * leaf){
				//Vector to return
				vector<int> path;
				//Pointer
				Node * curr = leaf;
				
				//Starting where the leaf is
				//Push the value (dir) into a vector
				//Point it to its parent and continue
				//Until its at the top
				//Issue: This means the encoded path is reversed
				while(curr != this->root){
					//Push the direction (0 or 1)
					path.push_back(curr->dir);
					curr = curr->parent;
				}

				//Print path for debugging purposes
				/*
				for(int i = 0; i < path.size(); i++){
					cout << path[i];
				}
				cout << leaf->symbol<< endl;
				*/
				return path;
			}

			//Function Compress grabs the path and prints it
			//in its correct order for the symbol its found
			//in the main file vs the tree
			void compress(char symbol, stream & out){
				int index = -1;
				//Get symbol position
				for(int i = 0; i < leaves.size(); i++){
					if(leaves[i]->symbol == symbol){
						index = i;
					}
				}

				//stray characters in the file from transfer
				//over from Windows to Linux
				//From earlier: path is reversed, so print it in reverse
				if(index == -1) return;
				vector<int> path = leaves[index]->encodedPath;
				for(int i = 0; i < path.size(); i++){
					out.bit(path[path[path.size()-i-1]]);
				}
			}

			//Constructing the Huffman tree, wikipedia algorithm
			//Use of STL container priority queue and vectors of nodes
			void create(const vector<int> & freqs){
				//Creating a vector of nodes
				for(int i = 0; i < freqs.size(); i++){
					//remove 0 frequency values
					if(freqs[i] > 0){
						//cout << (char)i << " " << freqs[i] << endl;
						Node * leaf = new Node((char)i,freqs[i],0,0,0);
						leaves.push_back(leaf);
					}
				}

				//Create priority queue of leaves
				//Compare is a public class function for comparison
				priority_queue<Node*,vector<Node*>,compare> pq;
				for(int i = 0; i < leaves.size(); i++){
					pq.push(leaves[i]);
				}

				//Testing pqueue
				//debugging if comparisons are working
				/*
				while(pq.size() > 1){
					Node * x = pq.top();		
					cout << x->freq << endl;
					pq.pop();
				}
				*/

				//Continue to construct and pop until 1 which means tree is complete
				while(pq.size() > 1){
					//Grab the top 2 smallest frequency nodes
					Node * a = pq.top();		
					pq.pop();
					Node * b = pq.top();		
					pq.pop();

					//Create a node with total weight of the two leaves
					int totalWeight = a->freq + b->freq;
					//Node(symbol,tWeight, left - 0, right - 1, parent)
					Node * it = new Node(0,totalWeight,b,a,0);

					//Set 0 or 1
					//Set the new node as the parent of the 2 nodes
					a->dir = 1;
					a->parent = it;
					b->dir = 0;
					b->parent = it;

					//Put 'it' into the pq
					pq.push(it);
				}

				//Point root to the top of pq
				root = pq.top();

				//Get the encode from direction (1 or 0)
				//for each ascii value in leaves
				for(int i = 0; i < leaves.size(); i++){
					leaves[i]->encodedPath = path(leaves[i]);
					//cout << leaves[i]->symbol << endl;
				}

				/* Avg
				int count = 0;
				for(int i = 0; i < leaves.size(); i++){
					count += leaves[i]->encodedPath.size();	
				}
				cout << count/leaves.size();
				*/
			}
};

int main(){
	ifstream file;
	file.open("Amazon_EC2.txt");
	if(!file.is_open()) cout << "File not found";

	//Create a vector of freqs
	//256 for ascii values initialized to 0
	vector<int> freq(256,0);

	int c;
	int count = 0;
	while(file.good()){
		count++;
		c = file.get();
		//End of file
		if(c < 0) continue;
		freq[c] = freq[c] + 1;
	}
	file.close();

	/*
	for(int i = 0; i < freq.size(); i++){
		if(freq[i] != 0){
			double ratio = ((double)freq[i]/(double)count)*100;
			cout << (char)i <<  " " << ratio << endl;
		}
	}
	*/

	//Instantiate tree
	HuffmanTree tree;
	//Call create to make the tree 
	tree.create(freq);

	//Create output file stream
	ofstream outputFile("compressedFile.txt",ios::out);	
	stream out(outputFile);

	//Open file to get letters to compress with path
	ifstream file2;
	file2.open("Amazon_EC2.txt");

	//Call compress to write bits of each letter to the file
	char z;
	while(file2.good()){
		z = (char)file2.get();	
		tree.compress(z, out);
	}

	file2.close();
	outputFile.close();
	cout << endl << "Outputfile Created: compressedFile.txt" << endl << endl;
	return 0;	
}
