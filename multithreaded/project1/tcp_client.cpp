#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <chrono>
#include <iomanip>

using namespace std;

static int sock_global = -1;

void* rx_loop(void*) {
    char message[1024] = {0};
    for (;;) {
        int n = read(sock_global, message, sizeof(message)-1);
        if (n <= 0) break;
        message[n] = '\0';
        auto t  = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        cout << "[" << std::put_time(std::localtime(&t), "%H:%M:%S") << "] " << message << endl;
        memset(message, 0, sizeof(message));
    }
    return nullptr;
}

int main() {
    int sock = 0;
    char buffer[1024] = {0};
    struct sockaddr_in ServerAddr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cout << "Socket creation error!" << endl;
        exit(EXIT_FAILURE);
    }

    cout << "Please enter TCP server IP Address: ";
    cin.getline(buffer, 1024, '\n');

    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port   = 0; // will set below
    if (inet_pton(AF_INET, buffer, &ServerAddr.sin_addr) <= 0) {
        cout << "Invalid address" << endl;
        exit(EXIT_FAILURE);
    }
    cout << "Please enter the server listening port: ";
    cin.getline(buffer, 9, '\n');
    ServerAddr.sin_port = htons(atoi(buffer));

    if (connect(sock, (struct sockaddr*)&ServerAddr, sizeof(ServerAddr)) < 0) {
        cout << "Connection failed!!" << endl;
        exit(EXIT_FAILURE);
    }

    sock_global = sock;

    // Start receiver thread
    pthread_t rx;
    pthread_create(&rx, nullptr, rx_loop, nullptr);
    pthread_detach(rx);

    // Sender loop (stdin)
    for (;;) {
        char line[1024] = {0};
        cout << "> ";
        if (!cin.getline(line, sizeof(line), '\n')) break;
        if (strlen(line) == 0) continue;
        if (send(sock, line, strlen(line), 0) == -1) {
            cout << "Send failed!" << endl;
            break;
        }
        if (strcmp(line, "Quit") == 0) break; // optional exit
    }

    close(sock);
    cout << "Exit..." << endl;
    return 0;
}

