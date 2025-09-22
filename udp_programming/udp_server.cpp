#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

int main(){
	int udpSocket, nBytes;
	char buffer[1024];
	struct sockaddr_in ServerAddr, ClientAddr;
	char *ClientIP;
	socklen_t addr_size;

	udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
	ServerAddr.sin_family = AF_INET;
	cout << "Please enter the listening port: ";
	cin.getline(buffer, 9, '\n');
	ServerAddr.sin_port = htons(atoi(buffer));
	ServerAddr.sin_addr.s_addr = INADDR_ANY;
	memset(ServerAddr.sin_zero, '\0', sizeof(ServerAddr.sin_zero));
	addr_size = sizeof(ServerAddr);
	bind(udpSocket, (struct sockaddr *)&ServerAddr, addr_size);
	addr_size = sizeof(ClientAddr);

	do {
		nBytes = recvfrom(udpSocket, buffer, 1024, 0, (struct sockaddr *)& ClientAddr, &addr_size);
		buffer[nBytes] = '\0';
		ClientIP = inet_ntoa(ClientAddr.sin_addr);
		cout << ClientIP << " says: " << buffer << endl;
	}while(strcmp(buffer, "Quit") != 0);
	close(udpSocket);
	return 0;
}
