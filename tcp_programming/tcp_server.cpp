#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

int main() {
	int server_fd, new_socket, nBytes, port, opt = 1;
	char buffer[1024] = {0};
	struct sockaddr_in ServerAddr;
	int addrlen = sizeof(ServerAddr);

	if(server_fd = socket(AF_INET, SOCK_STREAM, 0) == 0){
		cout << "Socket creation error!" << endl;
		exit(EXIT_FAILURE);
	}
	if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
		cout << "Socket setsocketopt error!" << endl;
		exit(EXIT_FAILURE);
	}
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.s_addr = INADDR_ANY;
	cout << "Please enter the listening port: ";
	cin.getline(buffer, 9, '\n');
	ServerAddr.sin_port = htons(atoi(buffer));
	if(bind(server_fd, (struct sockaddr *)&ServerAddr, sizeof(ServerAddr)) < 0){
		cout << "Bind failed!!" << endl;
		exit(EXIT_FAILURE);
	}
	cout << "Listening..." << endl;
	if(listen(server_fd, 3) < 0){
		cout << "Listen failure!" << endl;
		exit(EXIT_FAILURE);
	}
	if(new_socket = accept(server_fd, (struct sockaddr *)&ServerAddr, (socklen_t *)&addrlen) < 0){
		cout << "Accept Failed!" << endl;
		exit(EXIT_FAILURE);
	}
	do{
		nBytes = read(new_socket, buffer, 1024);
		buffer[nBytes] = '\0';
		cout << "I received: " << buffer << endl;
		if(send(new_socket, buffer, strlen(buffer), 0) == -1){
			cout << "Send failed!" << endl;
			close(new_socket);
			exit(EXIT_FAILURE);
		}
	}while(strcmp(buffer, "Quit") != 0);
	close(new_socket);
	cout << "Exit..." << endl;
	return 0;
}


