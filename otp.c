/***********************************************************
 * Name: Robert Jones
 * Email: jonesro4@oregonstate.edu
 * Class: CS 344 Spring 2020
 * Assignment: Program 4 - Dead Drop
 * File: OTP.C
 * *********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <fcntl.h>
#include <sys/stat.h>

#define MAX_BUFFER 256

void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues
void encrypt(char buffer[MAX_BUFFER], char kbuffer[MAX_BUFFER]);
void decrypt(char buffer[MAX_BUFFER], char kbuffer[MAX_BUFFER]);
int connect_to_server( int portNumber );
void send_message( int socketFD, char buffer[MAX_BUFFER], int bufferSize );
int read_message( int socketFD, char buffer[MAX_BUFFER] );

int main(int argc, char *argv[])
{
	if ( argc < 5 )
	{
		fprintf(stderr, "USAGE: %s post user plaintext key port\nUSAGE: %s get user key port\n", argv[0], argv[0]);
		exit(EXIT_FAILURE);
	}
	else if (strcmp(argv[1], "get") != 0 && strcmp(argv[1], "post") != 0) {
		fprintf(stderr, "USAGE: %s post user plaintext key port\nUSAGE: %s get user key port\n", argv[0], argv[0]);
		exit(EXIT_FAILURE);
	}
	else if (strcmp(argv[1], "post") == 0 && argc < 6) {
		fprintf(stderr, "USAGE: %s post user plaintext key port\nUSAGE: %s get user key port\n", argv[0], argv[0]);
		exit(EXIT_FAILURE);	
	}


	if (strcmp(argv[1], "post") == 0) 
	{
		char* user = argv[2];
		char* plaintextFile = argv[3];
		char* keyFile = argv[4];
		int port = atoi(argv[5]);
		int i = 0;
		int pChar = 0;
		int kChar = 0;
		
		int socketFD = connect_to_server(port);

		char buffer[MAX_BUFFER + 1];
		memset(buffer, '\0', sizeof(buffer));

		char kBuffer[MAX_BUFFER + 1];
		memset(kBuffer, '\0', sizeof(kBuffer));
		
		FILE * pFP = fopen(plaintextFile, "r");
		if (pFP == NULL) { 
			fprintf(stderr,"OTP: cannot open plaintext file.\n"); 
			exit(EXIT_FAILURE); 
		}

		FILE * kFP = fopen(keyFile, "r");
		if (kFP == NULL) { 
			fprintf(stderr, "OTP: cannot open keyfile.\n"); 
			exit(EXIT_FAILURE); 
		}

		fseek(pFP, 0L, SEEK_END);
		int totalMessageSize = ftell(pFP);
		rewind(pFP);
		
		send_message(socketFD, "post", 4);
		send_message(socketFD, user, strlen(user));

		sprintf(buffer, "%d", totalMessageSize-1);
		send_message(socketFD, buffer, strlen(buffer));
		
		while(1)
		{
			memset(buffer, '\0', sizeof(buffer));
			memset(kBuffer, '\0', sizeof(kBuffer));

			i = 0;
			while ( (pChar = fgetc(pFP)) != EOF && i < MAX_BUFFER) 
			{
				if ( pChar != 32 && (pChar < 65 || pChar > 90) ) {
					fprintf(stderr, "OTP: invalid character in plaintext file\n");
					exit(EXIT_FAILURE);
				}
				if ( kChar != 32 && (kChar < 65 || kChar > 90) ) {
					fprintf(stderr, "OTP: invalid character in keyfile \n");
					exit(EXIT_FAILURE);
				}
				if ( (kChar = fgetc(kFP)) == EOF ) {
					fprintf(stderr, "OTP: keyfile not long enough\n");
					exit(EXIT_FAILURE);
				}
				buffer[i] = (char)pChar;
				kBuffer[i] = (char)kChar;
				i++;
			}
			if (i == 0)
				break;
			encrypt(buffer, kBuffer);

			send_message(socketFD, buffer, strlen(buffer));
		}
		fclose(pFP);
		fclose(kFP);
		close(socketFD);
		
	} 
	else if (strcmp(argv[1], "get") == 0)
	{ 

		char* user = argv[2];
		char* keyFile = argv[3];
		int port = atoi(argv[4]);
		int kChar = 0;
		int charsRead = 0;
		
		int socketFD = connect_to_server(port);

		char buffer[MAX_BUFFER+1];
		memset(buffer, '\0', sizeof(buffer));
		char kbuffer[MAX_BUFFER+1];
		memset(kbuffer,'\0', sizeof(kbuffer));

		send_message(socketFD, "get", 3);		

		send_message(socketFD, user, strlen(user));	
		
		FILE * kFP = fopen(keyFile, "r");
		if (kFP == NULL) { 
			fprintf(stderr, "OTP cannot open keyfile.\n"); 
			exit(1); 
		}

		charsRead = read_message(socketFD, buffer);
		int messageLength = atoi(buffer);
	
		if (messageLength == 0) {
			fprintf(stderr, "OTP user does not have any messages\n");
			exit(EXIT_FAILURE);
		}

		int totalCharsRead = 0;
		
		while ( totalCharsRead < messageLength ) 
		{
			memset(buffer, '\0', MAX_BUFFER+1);
			memset(kbuffer, '\0', MAX_BUFFER+1);
			
			int charsRead = read_message(socketFD, buffer);
			
			totalCharsRead += charsRead;
			int i = 0;
			while ( i < charsRead && (kChar = fgetc(kFP)) != EOF) 
			{
				kbuffer[i] = (char)kChar;
				i++;
			}
			if (i == 0)
				break;

			decrypt(buffer, kbuffer);
			printf("%s", buffer);	
			
		}
		fclose(kFP);
		close(socketFD);	
	}

	return 0;
}

void encrypt(char buffer[MAX_BUFFER], char kbuffer[MAX_BUFFER])
{
	int i = 0;
	while ( buffer[i] != '\0' && buffer[i] != '\n' && i < MAX_BUFFER)
	{
		int charNum = 0;
		if ( buffer[i] == ' ' ) 
		{
			if ( kbuffer [i] == ' ' ) 
				charNum = (26 + 26) % 27;
			else 
				charNum = (26 + ( (int)kbuffer[i] - 65 ) ) % 27;
		} 
		else 
		{
			if ( kbuffer[i] == ' ' ) 
				charNum = ( ((int)buffer[i] - 65) + 26) % 27;
			else 
				charNum = ( ((int)buffer[i] - 65) + ((int)kbuffer[i] - 65) ) % 27;	
		}
		if ( charNum == 26 ) 
			buffer[i] = ' ';
		else 
			buffer[i] = (char)(charNum + 65);
		i++;
	}

}

void decrypt(char buffer[MAX_BUFFER], char kbuffer[MAX_BUFFER])
{
	int i = 0;
	while( buffer[i] != '\0' && buffer[i] != '\n' && i < MAX_BUFFER)
	{
		int charNum = 0;

		if ( buffer[i] == ' ' ) 
		{
			if ( kbuffer[i] == ' ' ) 
				charNum = (26 - 26) % 27;
			else 
				charNum = (26 - ( (int)kbuffer[i] - 65 ) ) % 27;
		} 
		else 
		{
			if ( kbuffer[i] == ' ' ) 
				charNum = ( ((int)buffer[i] - 65) - 26) % 27;
			else 
				charNum = ( ((int)buffer[i] - 65) - ((int)kbuffer[i] - 65) ) % 27;	
		}
		if ( charNum < 0 ) 
			charNum += 27;
		
		if ( charNum == 26 ) 
			buffer[i] = ' ';
		else 
			buffer[i] = (char)(charNum + 65);
		i++;
	}
}

int connect_to_server(int portNumber)
{


	int socketFD;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
    
	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	
	serverAddress.sin_family 	= AF_INET; // Create a network-capable socket
	serverAddress.sin_port 		= htons(portNumber); // Store the port number
	// Convert the machine name into a special form of address
	serverHostInfo 			= gethostbyname("localhost"); 
	if (serverHostInfo == NULL) { 
		fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
		exit(2); 
	}
	
	
	// Copy in the address
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); 

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) {
		fprintf(stderr, "CLIENT: ERROR opening socket\n");
		exit(2);
	}
	
	// Connect to server
	// Connect socket to address
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
		fprintf(stderr,"CLIENT: ERROR connecting\n");
		exit(2);
	}
	
	return socketFD;
}




int read_message( int socketFD, char buffer[MAX_BUFFER])
{
	int charsRead = 0;
	charsRead = recv( socketFD, buffer, MAX_BUFFER, 0);

	if ( charsRead == -1 )
		error ("failed to receive.");

	int i = 1;
	int charsWritten = send(socketFD, &i, sizeof i, 0);
		if (charsWritten < 0)
			error("ERROR acknowledgement not sent");
	return charsRead;
}

void send_message( int socketFD, char buffer[MAX_BUFFER], int bufferSize )
{
	// Send message to server
	int charsWritten = send(socketFD, buffer, bufferSize, 0); // Write to the server
	if (charsWritten < 0) 
		error("CLIENT: ERROR writing to socket");
	if (charsWritten < bufferSize) 
		printf("CLIENT: WARNING: Not all data written to socket!\n");
	int i = 0;
	while (i != 1)
		recv(socketFD, &i, sizeof i, 0);

	return;
}
