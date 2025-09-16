#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

int main() {
	int sock = 0, nBytes;
	char buffer[1024] = {0}, message[1024] = {0};
	struct sockaddr_in ServerAddr;
	int addrlen = sizeof(ServerAddr);

	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		cout << "Socket creation error!" << endl;
		exit(EXIT_FAILURE);
	}
	cout << "Please enter TCP server IP Address:";
	cin.getline(buffer, 1024, '\n');
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.s_addr = INADDR_ANY;
	cout << "Please enter the server listening port: ";
	cin.getline(buffer, 9, '\n');
	ServerAddr.sin_port = htons(atoi(buffer));
	if(connect(sock, (struct sockaddr *)&ServerAddr, sizeof(ServerAddr)) < 0){
		cout << "Connection failed!!" << endl;
		exit(EXIT_FAILURE);
	}
	
	do{
		memset(buffer, 0, sizeof(buffer));
		cout << "Message to server:";
		cin.getline(buffer, 1024, '\n');
		
		if(send(sock, buffer, strlen(buffer), 0) == -1){
			cout << "Send failed!" << endl;
			close(sock);
			exit(EXIT_FAILURE);
		}
		memset(message, 0, sizeof(message));
		read(sock, message, 1024);
		cout << "Server replied: " << message << endl;
	}while(strcmp(buffer, "Quit") != 0);
	close(sock);
	cout << "Exit..." << endl;
	return 0;
}


