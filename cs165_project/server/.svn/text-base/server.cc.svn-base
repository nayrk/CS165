#include <iostream>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fstream>
#include <openssl/sha.h>

using namespace std;

//Example From - 
//http://ardoino.com/20-openssl-rsa-implementation/
//Article && sources by Paolo Ardoino AKA binduck
int privateEnc(RSA * key, unsigned char * plain, int len, unsigned char **cipher){
	//Encrypt with private key, signing
	int n = RSA_private_encrypt(len, plain, *cipher, key, RSA_PKCS1_PADDING);
	if(n == -1){
		cout << "Failed to private encrypt";
	}
	return n;
}

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

//Loads private key
RSA * load_pk(){
	//Load Private Key
	FILE * fp = fopen("./rsa/private.key","rb");
	if(fp == NULL){
		cout << "Failed to open private.key\n";
		exit(1);
	}
	RSA * priv_key = PEM_read_RSAPrivateKey(fp,NULL,NULL,NULL);
	return priv_key;
}

//Loads public key
RSA * load_pubk(){
	//Open the servers' public key 
	FILE * fp = fopen("./rsa/public.key","rb");
	if(fp == NULL){
		cout << "Failed to open public.key\n";
		exit(1);
	}
	RSA * pub_key = PEM_read_RSA_PUBKEY(fp,NULL,NULL,NULL);
	return pub_key;
}

void serve(int client, int sock,struct sockaddr_in client_addr,int sin_size, string operation)
{
	while(1)
	{

		//Set up SSL CTX with Server Method
		SSL_CTX * ctx = SSL_CTX_new(SSLv23_server_method());
		//Allow ADH
		SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

		//Load Diffie-HEllman key exchange
		BIO * bio = BIO_new_file("dh1024.pem","r");
		if(bio == NULL){
			cerr << "Failed to load ADH pem";
			break;
		}

		//Read DH
		DH * ret = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
		BIO_free(bio);
		//Set up SSL_CTX 
		if(SSL_CTX_set_tmp_dh(ctx, ret) < 0){
			cerr << "Failed to set DH parameters";
			break;
		}

		//Generate RSA key
		RSA * rsa = RSA_generate_key(1024, RSA_F4, NULL, NULL);
		if(!(SSL_CTX_set_tmp_rsa(ctx, rsa))){
			cerr << "Failed to set RSA key";
			break;
		}
		RSA_free(rsa);

		//Set up SSL object
		SSL_CTX_set_cipher_list(ctx, "ADH");
		SSL * ssl = SSL_new(ctx);
		//Set BIO to use client socket, connected client
		BIO * sslclient = BIO_new_socket(client, BIO_NOCLOSE);
		SSL_set_bio(ssl, sslclient, sslclient);
		
		//Handshake
		int r = SSL_accept(ssl);
		if( r != 1){
			printf("Done.\nClosing connection for (%s , %d)", inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port)); 
			break;
		}
		//Verify Certificate
		if(SSL_get_verify_result(ssl) != X509_V_OK){
			cerr << "Certificate failed verification!";
			break;
		}

		//Wait for connection
		printf("I got a connection from (%s , %d)\n", inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port)); 

		//Read encrypted challenge from client side
		char buffer[2024];
		memset(buffer, 0, 2024);
		SSL_read(ssl,buffer,sizeof(buffer)-1);
		cout << "Received encrypted challenge " << endl;

		//Write hashed to file
		//Write size to file
		ofstream file;
		//Use client address for uniqueness in naming file
		string sname = inet_ntoa(client_addr.sin_addr);
		file.open(sname.c_str());
		file << buffer;
		file.close();

		//Initializtion of keys and variables
		int size=0,len=0;
		FILE *fpin=NULL;
		unsigned char *cipher=NULL,*plain=NULL;

		RSA * pk = load_pubk();
		RSA * privk = load_pk();
		int ks = RSA_size(pk);
		plain = (unsigned char *)malloc(ks * sizeof(unsigned char));
		cipher = (unsigned char*)malloc(ks * sizeof(unsigned char));

		fpin = fopen(sname.c_str(),"r");
		while(!feof(fpin)) {
			//Break when there are 0 bytes read
			//Set null termination chars
			memset(plain, '\0', (ks * sizeof(unsigned char)));
			memset(cipher, '\0', (ks * sizeof(unsigned char)));
			len = fread(cipher, 1, RSA_size(pk), fpin);
			//Decrypt the file
			//Confirm you have read everything
			cout << "Decrypting the clients challenge" << endl;
			if(len == RSA_size(pk))
			{
				decrypt(privk, cipher, RSA_size(pk), &plain);
				//cout << "Decrypted on Server Side -> Plain: " << plain << endl;

				unsigned char * it;
				it = (unsigned char *)malloc(RSA_size(pk) * sizeof(unsigned char *));
				//SHA1Hash
				unsigned char * SHA1Hash;
				SHA1Hash = (unsigned char *)malloc(20 * sizeof(unsigned char *));
				SHA1(plain, sizeof(it), SHA1Hash);
				cout << "Hashing the un-encrypted challenge with SHA1" << endl;

				//Private encrypt
				//ks = RSA_size(privk);
				unsigned char * hashedChallenge = NULL;
				hashedChallenge = (unsigned char *)malloc(ks * sizeof(unsigned char));
				memset(hashedChallenge, '\0', (ks * sizeof(unsigned char)));
				long bytesEncrypted = privateEnc(privk,SHA1Hash, (ks * sizeof(unsigned char)) - 11, &hashedChallenge);

				cout << "Signing the SHA1 hash" << endl;
				SSL_write(ssl, hashedChallenge, (20 * ks * sizeof(unsigned char *))); 
				cout << "Sending the signed hash to the client" << endl;
				break;
			}
			break;
		}
		fclose(fpin);

		if(operation == "store"){
			//Read file name
			memset(buffer, 0, 1024);
			SSL_read(ssl,buffer,sizeof(buffer)- 1);
			cout << "Storing file " << buffer << endl;
			
			//Write to file name
			filebuf fb;
			fb.open(buffer,ios::out);
			ostream os(&fb);
			while(1){
				memset(buffer, 0, 1024);
				int r = SSL_read(ssl, buffer, sizeof(buffer) - 1);
				os << buffer;
				int pending = SSL_pending(ssl);
				if(pending <= 0) break;
				if(r <= 0) break;
			}
			fb.close();

			//Close after file is read completely
			SSL_shutdown(ssl);
			SSL_free(ssl);
			SSL_CTX_free(ctx);
			close(client);
		}else if(operation == "retrieve")
		{
			//Read file to send
			memset(buffer, 0, 1024);
			SSL_read(ssl,buffer,sizeof(buffer)- 1);
			cout << "Sending Client File: " << buffer << endl;

			//Send file
			int length;
			char * b;
			ifstream is(buffer, ios::binary);
			is.seekg(0, ios::end);
			length = is.tellg();
			is.seekg(0, ios::beg);
			b = new char[length];
			is.read(b, length);
			is.close();
			SSL_write(ssl, b, length);

			//Close after file is sent completely
			SSL_shutdown(ssl);
			SSL_free(ssl);
			SSL_CTX_free(ctx);
			close(client);
		}
		string remove = "rm " + sname;
		system(remove.c_str());
	}
	//Remove fille
	close(sock);
}

int main(int argc, char *argv[]){
	//Initialize SSL	
	CRYPTO_malloc_init(); // Initialize malloc, free, etc for OpenSSL's use
	SSL_library_init(); // Initialize OpenSSL's SSL libraries
	SSL_load_error_strings(); // Load SSL error strings
	ERR_load_BIO_strings(); // Load BIO error strings
	OpenSSL_add_all_algorithms(); // Load all available encryption algorithms

	if(argc != 4){
			USAGE:
			fprintf(stderr,
					"Usage: %s "
					"[-port <port>] "
					"<operationName> store/retrieve "
					"\n",
					argv[0]);
			return (1);
	}
	if((string)argv[1] != "-port") goto USAGE;
	int port = atoi(argv[2]);
	string operation = (string)argv[3];
	if(operation != "store" && operation != "retrieve") goto USAGE;

	//Start Server
	int sock, client;
	struct sockaddr_in server_addr,client_addr;    

	//Server socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);
	bzero(&(server_addr.sin_zero),8); 

	//Bind to the port
	if(bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1){
		cerr << "Unable to bind socket";
		exit(1);
	}

	//Listen for up to 10 connections
	if(listen(sock,10) == -1) {
		cerr << "Failing to listen to 5";
		exit(1);
	}
	cout << "Server waiting for client(s) on port " << port << endl;
	socklen_t sin_size = sizeof(struct sockaddr);


	//Listening for cients
	//Fork to allow multiple clients
	pid_t pid;
	while(1){
		client = accept(sock, (struct sockaddr *)&client_addr,&sin_size);
		if(client < 0){
			cerr << "Failed to accept";
			return 1;	
		}
		if((pid=fork())){
			close(client);
		}else{
			serve(client,sock,client_addr,sin_size,operation);
			exit(0);
		}
	}
	return 0;
}
