#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define MAX_BUFFER 257

int receive_message(int establishedConnectionFD, char buffer[MAX_BUFFER]);
void send_message(int establishedConnectionFD, char buffer[MAX_BUFFER], int bufferSize);


void error(const char *msg) { 
	perror(msg); 
	exit(1); 
} // Error function used for reporting issues

void sigchld_handler(int s) {
	int saved_errno = errno;
	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}

int main(int argc, char *argv[])
{
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	char buffer[MAX_BUFFER];
	struct sockaddr_in serverAddress, clientAddress;
	struct sigaction sa;


	if (argc < 2) { 
		fprintf(stderr,"USAGE: %s port\n", argv[0]); 
		exit(1); 
	} // Check usage & args

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) 
		error("ERROR opening socket");

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("ERROR on binding");
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	while(1) {
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
	
		if (establishedConnectionFD < 0) {
			error("ERROR on accept");
			continue;
		}

		if (!fork()) {
			/****** Grading requirement ******/
			sleep(2);
			/*********************************/

			close(listenSocketFD);

			// Get the message from the client and display it
			memset(buffer, '\0', MAX_BUFFER);
			
			// Read the client's first message of get/post
			
			receive_message(establishedConnectionFD, buffer);
			

			if (strcmp(buffer, "post") == 0) 
			{
				
				memset(buffer, '\0', MAX_BUFFER);

				receive_message(establishedConnectionFD, buffer);
		
			
				FILE * fp;
				fp = fopen(buffer, "w");

				memset(buffer, '\0', MAX_BUFFER);
				receive_message(establishedConnectionFD, buffer);
				
				int messageLength = atoi(buffer);
				
				int totalCharsRead = 0;
				while (totalCharsRead < messageLength) {
					memset(buffer, '\0', MAX_BUFFER);
					
					charsRead = receive_message(establishedConnectionFD, buffer);
					
					if (charsRead == -1) 
						error("failed to receive.");
					totalCharsRead += charsRead;
					fprintf(fp, "%s", buffer);
				}
				fclose(fp);

			} 
			else if (strcmp(buffer, "get") == 0) 
			{
			
				int kChar = 0;
		
				memset(buffer, '\0', MAX_BUFFER);

				receive_message(establishedConnectionFD, buffer);


				FILE * fp;
				fp = fopen(buffer, "r");
				if (fp == NULL) { 
					fprintf(stderr, "cannot open user file.\n"); 
					send_message(establishedConnectionFD, "0", 1);
					exit(EXIT_FAILURE);
				}

				fseek(fp, 0L, SEEK_END);
				int totalMessageSize = ftell(fp);
				rewind(fp);

				memset(buffer, '\0', sizeof(buffer));
				sprintf(buffer, "%d", totalMessageSize);
				send_message(establishedConnectionFD, buffer, strlen(buffer)+1);

				while(1)
				{
					memset(buffer, '\0', sizeof(buffer));
					int i = 0;
					while( i < MAX_BUFFER -1 && (kChar = fgetc(fp)) != EOF)
					{
						buffer[i] = (char)kChar;
						i++;
					}
					if (i == 0)
						break;
					send_message(establishedConnectionFD, buffer, strlen(buffer));
				}			
			} 
			else 
			{
				error("ERROR unknown command get/post");
			}

			
			
			close(establishedConnectionFD);
			exit(0);
		}
		close(establishedConnectionFD); // Close the existing socket which is connected to the client
		
	}
	close(listenSocketFD); // Close the listening socket
	return 0; 
}

int receive_message(int establishedConnectionFD, char buffer[MAX_BUFFER])
{
	int charsRead = recv(establishedConnectionFD, buffer, MAX_BUFFER, 0); 
		if (charsRead < 0) 
			error("ERROR reading from socket");
	int i = 1;
	int charsWritten = send(establishedConnectionFD, &i, sizeof i, 0);
		if (charsWritten < 0)
			error("ERROR acknowledgement not sent");
	return charsRead;
}

void send_message(int establishedConnectionFD, char buffer[MAX_BUFFER], int bufferSize)
{
	int charsWritten = send(establishedConnectionFD, buffer, bufferSize, 0);
	if (charsWritten < 0)
		error("SERVER: ERROR writing to socket");
	if (charsWritten < bufferSize)
		printf("SERVER WARNING: Not all data written to socket\n");
	int i = 0;
	while (i != 1) 
		recv(establishedConnectionFD, &i, sizeof i, 0);

	return;
}
