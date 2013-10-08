#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "44444"

int main(int argc, char **argv) 
{
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	char sendbuf[DEFAULT_BUFLEN] = "this is a test";
	char recvbuf[DEFAULT_BUFLEN] = {0};
	char buf[DEFAULT_BUFLEN] = {0};
	char ID[DEFAULT_BUFLEN] = {0};
	char UserName[DEFAULT_BUFLEN] = {0};
	char Password[DEFAULT_BUFLEN+1] = {0};
	int iResult;
	int retry = 0;
	u_short pwlen = 0;

	// Validate the parameters
	if (argc != 2) 
	{
		printf("usage: %s server-name\n", argv[0]);
		return 1;
	}

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory( &hints, sizeof(hints) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
	if ( iResult != 0 ) 
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for(ptr = result; ptr != NULL; ptr = ptr->ai_next) 
	{
		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET)
		{
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET)
	{
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	//We're connected. Expect welcome message
	iResult = recv(ConnectSocket, recvbuf, DEFAULT_BUFLEN, 0);
	if ( iResult > 0 ) //If we received bytes
	{
		strcat(buf, recvbuf);
		if (strncmp(buf, "Welcome to The Server\n", 22) == 0)
		{
			printf("recv: Welcome to The Server\n");
		}
	}
	else if ( iResult == 0 )
	{
		printf("An error occured communicating with the server. Connection closed.\n");
		closesocket(ConnectSocket);
		WSACleanup();
		return 0;
	}
	else
	{
		printf("ERROR: recv failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 0;
	}

	//Prompt for ID (8 digit) and Username (20 max char)
	retry = 3;
	while(retry)
	{
		memset(buf, 0, DEFAULT_BUFLEN);
		printf("ID: ");
		scanf("%s", buf);
		if (strlen(buf) != 8)
		{
			printf("Please enter 8-digit ID.\n");
			continue;
		}
		if (!strIsDigit(buf))
		{
			printf("Please enter 8-digit ID.\n");
			continue;
		}
		strncpy(ID, buf, 8);

		memset(buf, 0, DEFAULT_BUFLEN);
		printf("Username: ");
		scanf("%s", buf);
		if (strlen(buf) > 20)
		{
			printf("Username must be shorter than 20 characters.\n");
			continue;
		}

		strncpy(UserName, buf, 20);
		//We have ID and username, send to server
		memset(sendbuf, 0, DEFAULT_BUFLEN);
		strncpy(sendbuf, ID, 8);
		strncat(sendbuf, "\n", 1);
		strncat(sendbuf, UserName, strlen(UserName));
		strncat(sendbuf, "\n", 1);
		iResult = send( ConnectSocket, sendbuf, (int)strlen(sendbuf), 0 );
		if (iResult == SOCKET_ERROR) 
		{
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();
			return 1;
		}

		//Look for "Success" or "Failure"
		iResult = recv(ConnectSocket, recvbuf, DEFAULT_BUFLEN, 0);
		if ( iResult > 0 ) //If we received bytes
		{
			if (strncmp(recvbuf, "Success", 7) == 0) //ID/UserName was good. Ask for password.
			{
				break;
			}
			else if (strncmp(recvbuf, "Failure", 7) == 0) //ID/UserName was bad. Retry
			{
				--retry;
				if (retry)
				{
					printf("Bad ID/Username. Try again.\n");
				}
				continue;
			}
		}
		else if ( iResult == 0 )
		{
			printf("An error occured communicating with the server. Connection closed.\n");
			closesocket(ConnectSocket);
			WSACleanup();
			return 1;
		}
		else
		{
			printf("ERROR: recv failed with error: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();
			return 1;
		}
	}

	if (!retry) //If all retries used, close connection
	{
		printf("Retry limit reached. Closing connection...\n");
		// shutdown the connection
		iResult = shutdown(ConnectSocket, SD_SEND);
		if (iResult == SOCKET_ERROR) 
		{
			printf("shutdown failed with error: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();
			return 1;
		}
		closesocket(ConnectSocket);	
		system("pause");
		return 0;
	}



	//Prompt for password and send
	retry = 3;
	while (retry)
	{
		printf("Password: ");
		scanf("%512s", Password);

		pwlen = htons((short)strlen(Password));

		iResult = send( ConnectSocket, (char*)&pwlen, 2, 0 ); //Send length of password
		if (iResult == SOCKET_ERROR) 
		{
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();
			return 1;
		}

		iResult = send( ConnectSocket, Password, strlen(Password), 0 ); //Send password
		if (iResult == SOCKET_ERROR) 
		{
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();
			return 1;
		}

		iResult = recv(ConnectSocket, recvbuf, DEFAULT_BUFLEN, 0);
		if ( iResult > 0 ) //If we received bytes
		{
			if (strncmp(recvbuf, "Congratulations", 15) == 0) //Password was good, display message and disconnect
			{
				recvbuf[iResult] = 0; //terminate message
				printf("recv: %s\n", recvbuf);
				break;
			}
			else if (strncmp(recvbuf, "Password incorrect", 7) == 0) //ID/UserName was bad. Retry
			{
				--retry;
				if(retry)
				{
					printf("Bad Password. Try again.\n");
				}
				continue;
			}
		}
		else if ( iResult == 0 )
		{
			printf("An error occured communicating with the server. Connection closed.\n");
			closesocket(ConnectSocket);
			WSACleanup();
			return 0;
		}
		else
		{
			printf("ERROR: recv failed with error: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();
			return 0;
		}

	}

	if (!retry) //If all retries used, alert user.
	{
		printf("Retry limit reached. Closing connection...\n");
	}

	// shutdown the connection 
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR)
	{
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	system("pause");

	return 0;
}

int strIsDigit(char* str)
{
	while (*str)
	{
		if (!isdigit(*str))
			return 0;
		else
			++str;
	}
	return 1;
}