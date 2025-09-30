#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <vector>
#include <algorithm>
#include <chrono>

using namespace std;

static pthread_mutex_t clients_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t count_mtx   = PTHREAD_MUTEX_INITIALIZER;
static vector<int> clients;
static int thread_count = 0;
static auto server_start = std::chrono::steady_clock::now();

void* respond(void* arg);

static void broadcast_except(int sender_fd, const std::string& msg) {
    pthread_mutex_lock(&clients_mtx);
    for (int fd : clients) {
        if (fd == sender_fd) continue;
        send(fd, msg.c_str(), msg.size(), 0);
    }
    pthread_mutex_unlock(&clients_mtx);
}

int main() {
    int server_fd, new_socket, opt = 1;
    char buffer[1024] = {0};
    struct sockaddr_in ServerAddr;
    socklen_t addrlen = sizeof(ServerAddr);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        cout << "Socket creation error!" << endl;
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        cout << "Socket setsocketopt error!" << endl;
        exit(EXIT_FAILURE);
    }

    ServerAddr.sin_family      = AF_INET;
    ServerAddr.sin_addr.s_addr = INADDR_ANY;
    cout << "Please enter the listening port: ";
    cin.getline(buffer, 9, '\n');
    ServerAddr.sin_port = htons(atoi(buffer));

    if (bind(server_fd, (struct sockaddr*)&ServerAddr, sizeof(ServerAddr)) < 0) {
        cout << "Bind failed!!" << endl;
        exit(EXIT_FAILURE);
    }
    cout << "Listening..." << endl;
    if (listen(server_fd, 64) < 0) {
        cout << "Listen failure!" << endl;
        exit(EXIT_FAILURE);
    }

    while (true) {
        new_socket = accept(server_fd, (struct sockaddr*)&ServerAddr, &addrlen);
        if (new_socket < 0) {
            cout << "Accept failed!!" << endl;
            continue;
        }

        // Track client socket
        pthread_mutex_lock(&clients_mtx);
        clients.push_back(new_socket);
        pthread_mutex_unlock(&clients_mtx);

        // Spawn handler thread
        pthread_t tid;
        pthread_create(&tid, nullptr, respond, new int(new_socket));
        pthread_detach(tid);

        // Update count
        pthread_mutex_lock(&count_mtx);
        thread_count++;
        cout << "New connection! Number of connections: " << thread_count << endl;
        pthread_mutex_unlock(&count_mtx);
    }
    return 0;
}

void* respond(void* arg) {
    int new_socket = *reinterpret_cast<int*>(arg);
    delete reinterpret_cast<int*>(arg);

    char buffer[1024] = {0};
    int nBytes = 0;

    while ( (nBytes = read(new_socket, buffer, sizeof(buffer) - 1)) > 0 ) {
        buffer[nBytes] = '\0';
        std::string line(buffer);

        // Commands:
        if (line.rfind("/say ", 0) == 0) {
            std::string text = line.substr(5);
            broadcast_except(new_socket, text);
        } else if (line == "/stats") {
            auto now  = std::chrono::steady_clock::now();
            auto secs = std::chrono::duration_cast<std::chrono::seconds>(now - server_start).count();

            int count_now;
            pthread_mutex_lock(&clients_mtx);
            count_now = (int)clients.size();
            pthread_mutex_unlock(&clients_mtx);

            std::string reply = "clients=" + std::to_string(count_now) +
                                " uptime_s=" + std::to_string(secs);
            send(new_socket, reply.c_str(), reply.size(), 0);
        } else {
            // Optional: echo fallback or ignore
            // send(new_socket, buffer, strlen(buffer), 0);
        }
        memset(buffer, 0, sizeof(buffer));
    }

    // Cleanup on disconnect
    pthread_mutex_lock(&clients_mtx);
    clients.erase(remove(clients.begin(), clients.end(), new_socket), clients.end());
    pthread_mutex_unlock(&clients_mtx);

    close(new_socket);

    pthread_mutex_lock(&count_mtx);
    thread_count--;
    cout << "Client disconnected. Connections: " << thread_count << endl;
    pthread_mutex_unlock(&count_mtx);

    pthread_exit(nullptr);
}
