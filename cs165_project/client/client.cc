#include <iostream>
#include <sys/stat.h>
#include <fstream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <time.h>
/* OpenSSL headers */
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/engine.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

using namespace std;
int decrypt(RSA *key, unsigned char *cipher, int len, unsigned char **plain)
{
	//Decrypt with length of private key, input cipher, output plain, private key, padding
	int n =  RSA_private_decrypt(len, cipher, *plain, key, RSA_PKCS1_PADDING);
	if (n == -1){
		cout << "Failed to decrypt";
		exit(1);
	}
	return n;
}

//Example From - 
//http://ardoino.com/20-openssl-rsa-implementation/
//Article && sources by Paolo Ardoino AKA binduck
int publicDec(RSA *key, unsigned char *cipher, int len, unsigned char **plain){
	int n =  RSA_public_decrypt(len, cipher, *plain, key, RSA_PKCS1_PADDING);
	if(n == -1){
		cout << "Failed to public decrypt";
		exit(1);
	}
	return n;
}

int encrypt(RSA *key, unsigned char *plain, int len, unsigned char **cipher)
{
	//Encrypt with bytes of plain text, input plaintext, output cipher, public key, padding
	int n = RSA_public_encrypt(len, plain, *cipher, key, RSA_PKCS1_PADDING);
	if (n == -1){
		cout << "Failed to encrypt";
		exit(1);
	}
	return n;
}

//Loads public key of client
RSA * load_client_pubk(){
	FILE * fp = fopen("./clientRSA/public.key","rb");
	if(fp == NULL){
		cout << "Failed to open client public.key\n";
		exit(1);
	}
	RSA * pub_key = PEM_read_RSA_PUBKEY(fp,NULL,NULL,NULL);
	return pub_key;
}

//Loads public key of the server
RSA * load_pubk(){
	//Open the servers' public key 
	FILE * fp = fopen("./serverRSA/public.key","rb");
	if(fp == NULL){
		cout << "Failed to open my own public.key\n";
		exit(1);
	}
	RSA * pub_key = PEM_read_RSA_PUBKEY(fp,NULL,NULL,NULL);
	return pub_key;
}

//Loads own private key for decrypting retrieved file
RSA * load_own_pk(){
	//Load Private Key
	FILE * fp = fopen("./clientRSA/private.key","rb");
	if(fp == NULL){
		cout << "Failed to open own client private.key\n";
		exit(1);
	}
	RSA * priv_key = PEM_read_RSAPrivateKey(fp,NULL,NULL,NULL);
	return priv_key;
}

//Initiate challenge function
//It seeds the RAND engine with great entropy
//It returns the hashed challenge and a reference to the original challenge for comparison
unsigned char * challenge(unsigned char ** original){
	//Encrypt challenge
	//Seed randomness with /dev/urandom
	FILE * pipe = popen("head /dev/urandom","r");
	if(!pipe){
		cout << "Failed to generate randomness from /dev/urandom";
		return false;
	}
	char buffer[128];
	string result = "";
	while(!feof(pipe)){
		if(fgets(buffer,128,pipe) != NULL){
			result += buffer;
		}
	}
	pclose(pipe);

	cout << "Seeding OpenSSL RAND_seed for PRNG" << endl;

	//Seed with result
	RAND_seed(result.c_str(),sizeof(result));

	//Load keys
	RSA * pk = load_pubk();
	int ks = 0;
	ks = RSA_size(pk);

	//Get random bytes with OpenSSL RAND_bytes
	unsigned char * random;
	random = (unsigned char *)malloc(ks * sizeof(unsigned char));
	memset(random, '\0', (ks * sizeof(unsigned char)));
	RAND_bytes(random, sizeof(random));

	//Encrypt the random bytes challenge
	unsigned char * hashedChallenge = NULL;
	hashedChallenge = (unsigned char *)malloc(ks * sizeof(unsigned char));
	memset(hashedChallenge, '\0', (ks * sizeof(unsigned char)));
	long bytesEncrypted = encrypt(pk,(unsigned char *)random, (ks * sizeof(unsigned char)) - 11, &hashedChallenge);

	//Set original to compare outside of function later
	*original = random;

	//Return a string of hashed challenge
	string str = (char *)hashedChallenge;

	cout << "Encrypted the challenge generated with RAND_Bytes" << endl;
	return hashedChallenge;
}


//Initiate ssl
void openssltest(string file,string address, string op) {
	//Set up SSL context object
	SSL_CTX* ctx = SSL_CTX_new(SSLv23_client_method());
	//Set cipher list to ADH 
	SSL_CTX_set_cipher_list(ctx, "ADH");
	//Allow ADH use
	SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

	SSL* ssl;
	//Set up bio for SSL connection
	BIO* bio = BIO_new_ssl_connect(ctx);
	//Check for error
	if (bio == NULL) {
		printf("Error creating BIO!\n");
		ERR_print_errors_fp(stderr);
		//Clean up SSL_CTX
		SSL_CTX_free(ctx);
		exit(1);
	}
	//Connecting bio and ssl object
	BIO_get_ssl(bio, &ssl);
	//Set mode to retry on failure
	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
	//Connecting to host
	BIO_set_conn_hostname(bio, address.c_str());
	//BIO_set_conn_hostname(bio, "localhost:4433");
	if(BIO_do_connect(bio) <= 0){
		printf("Failed to connect!");
		//Clean up BIO and SSL_CTX
		BIO_free_all(bio);
		SSL_CTX_free(ctx);
		exit(1);
	}

	//Handshake request
	if(BIO_do_handshake(bio) <= 0){
		printf("Failed SSL handshake!");
		BIO_free_all(bio);
		SSL_CTX_free(ctx);
		exit(1);
	}

	//Initiate challenge
	
	//Variable it is the un-encrypted challenge
	//Hash it with SHA1
	RSA * pk = load_pubk();
	unsigned char * it;
	it = (unsigned char *)malloc(RSA_size(pk) * sizeof(unsigned char *));

	unsigned char * ch = challenge(&it);
	//Send challenge
	BIO_puts(bio,(const char *)ch);

	unsigned char * SHA1Hash;
	SHA1Hash = (unsigned char *)malloc(20 * sizeof(unsigned char *));
	SHA1(it, sizeof(it), SHA1Hash);
	cout << "Hashing the un-encrypted challenge using SHA1" << endl;

	char r[1024];
	//Wait for response

	//Write encrypted SHA that was sent from server side
	ofstream SHAz;
	//Use unique name
	string sname = file + "SHA";
	SHAz.open(sname.c_str());
	int x = SSL_read(ssl, r, sizeof(r) - 1);
	SHAz << r;
	SHAz.close();

	FILE * fpin=NULL;
	fpin = fopen(sname.c_str(),"r");
	//fpin = fopen("encSHA.txt","r");
	unsigned char * cipher, * plain;
	cipher = (unsigned char *)malloc(RSA_size(pk) * sizeof(unsigned char *));
	plain = (unsigned char *)malloc(RSA_size(pk) * sizeof(unsigned char *));

	cout << "Reading the signed hash from the server" << endl;
	while(!feof(fpin)){
		memset(cipher, '\0', (RSA_size(pk) * sizeof(unsigned char *)));
		memset(plain, '\0', (RSA_size(pk) * sizeof(unsigned char *)));
	
		//Read encrypted SHA sent from server
		int len = fread(cipher,1, RSA_size(pk), fpin);
		if(len == RSA_size(pk)){
			//Decrypt encrypted SHA1Hash and compare
			long bytesUnEnc = publicDec(pk,(unsigned char *)cipher, RSA_size(pk), &plain);
			string mySHA = (char *)SHA1Hash;
			string serverSHA = (char *)plain;
			if(mySHA == serverSHA){
				cout << "Server challenge returned is correct." << endl;
			}
		}
		break;
	}
	cout << "Server challenge verified " << endl;
	fclose(fpin);
	free(cipher);
	free(plain);

	if(op == "store"){
		//Send file name
		char send[1024];
		memset(send, 0, sizeof(send));
		strcat(send, file.c_str());
		BIO_puts(bio,send);

		//Encrypt the file with the client public key
		//Load clients public key
		RSA * key = load_client_pubk();
		int ks = RSA_size(key);	

		//Create a unique name
		FILE * fpin, * fpout;
		string sname = "enc" + file;
		fpin = fopen(file.c_str(),"r");
		fpout = fopen(sname.c_str(),"w");

		unsigned char * cipher, * plain;
		cipher = (unsigned char *)malloc(ks * sizeof(unsigned char *));
		plain = (unsigned char *)malloc(ks * sizeof(unsigned char *));

		//Begin encryption and write to a file
		int len=0,size=0;
		while(!feof(fpin)){
			memset(plain,'\0',ks+1);
			memset(cipher,'\0',ks+1);
			len = fread(plain,1,ks-11,fpin);
			size = encrypt(key,plain,len,&cipher);
			fwrite(cipher,1,size,fpout);
		}
		fclose(fpout);
		fclose(fpin);

		//Send encrypted file
		int length;
		char * buffer;
		ifstream is(sname.c_str(), ios::binary);
		//ifstream is(file.c_str(), ios::binary);
		is.seekg(0, ios::end);
		length = is.tellg();
		is.seekg(0, ios::beg);
		buffer = new char[length];
		is.read(buffer,length);
		is.close();
		SSL_write(ssl, buffer, length);

		//Remove encrypted file
		string rm = "rm " + sname;
		system(rm.c_str());

		cout << "The server has received the file." << endl;

	}else if(op == "retrieve"){
		//Request for file by name
		char buffer[1024];
		memset(buffer, 0, sizeof(buffer));
		strcat(buffer, file.c_str());
		BIO_puts(bio,buffer);

		//Clear buffer
		BIO_flush(bio);

		char cname[1024];
		strcpy(cname,buffer);

		//Write to new file
		filebuf fb;
		fb.open(buffer,ios::out);
		ostream os(&fb);

		ifstream is(buffer, ios::binary);

		//Clear buffer by reading multiple times
		int x = 11;
		while(1){
			memset(buffer, '\0', 1024);
			int r = BIO_read(bio, buffer, sizeof(buffer) - 1);
			x--;
			if(x <= 0){
				os << buffer;
			}
			if(r <= 0) break;
		}
		fb.close();

		int length;
		char * b;
		is.seekg(0, ios::end);
		length = is.tellg();
		is.seekg(0, ios::beg);
		b = new char[length];
		is.read(b,length);
		is.close();

		RSA * privk = load_own_pk();
		int ks = RSA_size(privk);
		plain = (unsigned char *)malloc(ks * sizeof(unsigned char *));
		memset(plain,'\0',ks+1);
		decrypt(privk, (unsigned char *)b, length, &plain);
		cout << "\nDecrypted text: " << endl;
		cout << plain << endl;
		//Empty file before writing to it
		string empty = (char *)cname;
		empty = ">" + empty;
		system(empty.c_str());
		ofstream w(cname);
		w << plain;
		w.close();
	}
	//Remove SHA file
	string remove = "rm " + sname;
	system(remove.c_str()); 
	BIO_free_all(bio);
	SSL_CTX_free(ctx);
	return;
}

bool is_empty(std::ifstream& pFile)
{
    return pFile.peek() == std::ifstream::traits_type::eof();
}

int main(int argc, char * argv[])
{
	// Initializing OpenSSL 
	CRYPTO_malloc_init(); //Initialize OpenSSL's SSL libraries
	SSL_library_init(); // load encryption & hash algorithms for SSL
	SSL_load_error_strings(); // load error strings for error reporting
	ERR_load_BIO_strings(); // load error strngs for error reporting
	OpenSSL_add_all_algorithms();

	if(argc != 8){
			USAGE:
			fprintf(stderr,
					"Usage: %s "
					"[-server <address>] "
					"[-port <port>] "
					"[<filename>] "
					"[-operation <store/retrieve>] "
					"\n",
					argv[0]);
			return (1);
	}
	if((string)argv[1] != "-server") goto USAGE;
	if((string)argv[2] == "") goto USAGE;
	if((string)argv[3] != "-port") goto USAGE;
	if((string)argv[4] == "") goto USAGE;
	if((string)argv[5] == "") goto USAGE;
	if((string)argv[6] != "-operation") goto USAGE;
	if((string)argv[7] == "") goto USAGE;

	//Server Address, Port, Filename, Operation
	string h,p,f,o = "";
	h = (string)argv[2];
	p = (string)argv[4];
	f = (string)argv[5];
	o = (string)argv[7];

	if(o != "store" && o != "retrieve") goto USAGE;

	string address = h + ":" + p;
	//test if file is good/empty
	ifstream my_file(f.c_str(),ios::in | ios::binary);
	if(o == "store"){
		if(!my_file.good() || is_empty(my_file)){
			cerr << "File doesn't exist or is empty" << endl;
			return 1;
		}
	}
	my_file.close();

	cout << "Establishing SSL connection with the server" << endl;
	openssltest(f,address,o);
	return 0;
}
