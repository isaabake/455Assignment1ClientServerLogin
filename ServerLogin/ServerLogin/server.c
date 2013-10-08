#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "44444"

int main(void) 
{
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult;
	u_short pwlen;
	int retry = 0;
	char* pwlenchar = (char*)&pwlen;
	char recvbuf[DEFAULT_BUFLEN];
	char sendbuf[DEFAULT_BUFLEN];
	char buf[DEFAULT_BUFLEN];
	char ID[DEFAULT_BUFLEN];
	char UserName[DEFAULT_BUFLEN];


	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	while(1)
	{
		// Resolve the server address and port
		iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
		if ( iResult != 0 ) {
			printf("getaddrinfo failed with error: %d\n", iResult);
			WSACleanup();
			return 1;
		}

		// Create a SOCKET for connecting to server
		ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (ListenSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			freeaddrinfo(result);
			WSACleanup();
			return 1;
		}

		// Setup the TCP listening socket
		iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			printf("bind failed with error: %d\n", WSAGetLastError());
			freeaddrinfo(result);
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		freeaddrinfo(result);

		iResult = listen(ListenSocket, SOMAXCONN);
		if (iResult == SOCKET_ERROR) {
			printf("listen failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		// No longer need server socket
		closesocket(ListenSocket);

		//We're connected, display Welcome
		strcpy(sendbuf, "Welcome to The Server\n");
		iSendResult = send( ClientSocket, sendbuf, strlen(sendbuf), 0 );  //strlen() does not include null-terminator
		if (iSendResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}

		//Receive login
		retry = 3;
		while(retry) //Loop for retries
		{
			iResult = recv(ClientSocket, recvbuf, DEFAULT_BUFLEN, 0);
			if ( iResult > 0 ) //If we received bytes
			{
				iResult = sscanf(recvbuf, "%s\n%s\n", ID, UserName);
				if (iResult != 2) //If there aren't 2 "arguments"
				{
					printf("An error occured communicating with client - Could not get ID/Username.\nConnection closed.\n");
					closesocket(ClientSocket);
					WSACleanup();
					return 0;
				}
				//We have ID/Username, validate
				if (strncmp(ID, "12345678", 8) == 0 && strncmp(UserName, "isaac", iResult) == 0) //Good login
				{
					iSendResult = send( ClientSocket, "Success", 7, 0 );
					if (iSendResult == SOCKET_ERROR) 
					{
						printf("send failed with error: %d\n", WSAGetLastError());
						closesocket(ClientSocket);
						WSACleanup();
						return 1;
					}
					break;

				}
				else //Bad login
				{
					iSendResult = send( ClientSocket, "Failure", 7, 0 );
					if (iSendResult == SOCKET_ERROR) 
					{
						printf("send failed with error: %d\n", WSAGetLastError());
						closesocket(ClientSocket);
						WSACleanup();
						return 1;
					}
					--retry;
				}

			}
			else if ( iResult == 0 )
			{
				printf("An error occured communicating with client. Connection closed.\n");
				closesocket(ClientSocket);
				WSACleanup();
				return 0;
			}
			else
			{
				printf("ERROR: recv failed with error: %d\n", WSAGetLastError());
			}
		}

		if (!retry) //If all retries used, close connection
		{
			// shutdown the connection
			iResult = shutdown(ClientSocket, SD_SEND);
			if (iResult == SOCKET_ERROR) 
			{
				printf("shutdown failed with error: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}
			closesocket(ClientSocket);
			continue;
		}


		//Read Password
		retry = 3;
		while(retry) //Loop for retries
		{
			iResult = recv(ClientSocket, pwlenchar, 2, 0); //Get password length
			if ( iResult > 0 ) //If we received bytes
			{
				//Get length
				if (iResult == 2)
				{
					pwlen = ntohs(pwlen);
				}
				else
				{
					printf("An error occured communicating with client. Connection closed.\n");
					closesocket(ClientSocket);
					WSACleanup();
					return 1;
				}
			}
			else if ( iResult == 0 )
			{
				printf("An error occured communicating with client. Connection closed.\n");
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}
			else
			{
				printf("ERROR: recv failed with error: %d\n", WSAGetLastError());
			}

			iResult = recv(ClientSocket, recvbuf, pwlen, 0); //Get password
			if ( iResult > 0 ) //If we received bytes
			{
				if (strncmp(recvbuf, "baker", pwlen) == 0 ) //Good password
				{
					sprintf(buf, "Congratulations %s; you've just revealed the password for %s to the world", UserName, ID);
					iSendResult = send( ClientSocket, buf, strlen(buf), 0 );
					if (iSendResult == SOCKET_ERROR) 
					{
						printf("send failed with error: %d\n", WSAGetLastError());
						closesocket(ClientSocket);
						WSACleanup();
						return 1;
					}
					break;

				}
				else //Bad password
				{
					iSendResult = send( ClientSocket, "Password incorrect", 18, 0 );
					if (iSendResult == SOCKET_ERROR) 
					{
						printf("send failed with error: %d\n", WSAGetLastError());
						closesocket(ClientSocket);
						WSACleanup();
						return 1;
					}
					--retry;
				}
			}
			else if ( iResult == 0 )
			{
				printf("An error occured communicating with client. Connection closed.\n");
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}
			else
			{
				printf("ERROR: recv failed with error: %d\n", WSAGetLastError());
			}
		}


		// shutdown the connection
		iResult = shutdown(ClientSocket, SD_SEND);
		if (iResult == SOCKET_ERROR) 
		{
			printf("shutdown failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}

		// cleanup
		closesocket(ClientSocket);
	}


	WSACleanup();

	return 0;
}