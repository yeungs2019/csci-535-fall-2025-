#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

using namespace std;

int main(){
	int clientSocket, nBytes;
	char buffer[1024];
	struct sockaddr_in ServerAddr;
	socklen_t addr_size;

	clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
	ServerAddr.sin_family = AF_INET;
	cout << "Please enter the server port: ";
	cin.getline(buffer, 9, '\n');
	ServerAddr.sin_port = htons(atoi(buffer));
	cout << "Please enter the server IP: ";
	cin.getline(buffer, 16, '\n');
	ServerAddr.sin_addr.s_addr = inet_addr(buffer);
	memset(ServerAddr.sin_zero, '\0', sizeof(ServerAddr.sin_zero));
	addr_size = sizeof(ServerAddr);

	do {
		cout << "Type a sentence to send to the server: ";
		cin.getline(buffer, 1023, '\n');
		nBytes = strlen(buffer) + 1;
		sendto(clientSocket, buffer, nBytes, 0, (struct sockaddr *)& ServerAddr, addr_size);
	}while(strcmp(buffer, "Quit") != 0);
	return 0;
}
