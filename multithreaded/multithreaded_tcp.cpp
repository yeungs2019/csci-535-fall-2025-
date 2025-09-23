#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <vector>

using namespace std;
void *respond(void *arg);
int thread_count = 0;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;


int main()
{
    int server_fd, new_socket, nBytes, port, opt = 1;
    char buffer[1024]={0};
    struct sockaddr_in ServerAddr;
    int addrlen = sizeof(ServerAddr);
    pthread_t tid[100];

    if ((server_fd = socket(AF_INET,SOCK_STREAM, 0))==0)
    {
        cout << "Socket creation error!" << endl;
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, & opt, sizeof(opt)))
    {
        cout << "Socket setsocketopt error!" << endl;
        exit(EXIT_FAILURE);
    }
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_addr.s_addr = INADDR_ANY;
    cout << "Please enter the listening port: ";
    cin.getline(buffer, 9,'\n');
    ServerAddr.sin_port = htons(atoi(buffer));
    if (bind(server_fd, (struct sockaddr *)&ServerAddr, sizeof(ServerAddr)) < 0)
    {
        cout << "Bind failed!!" << endl;
        exit(EXIT_FAILURE);
    }
    cout << "Listening...." << endl;
    if (listen(server_fd, 3) < 0)
    {
        cout << "Listen failure!" << endl;
        exit(EXIT_FAILURE);
    }
    while(1)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&ServerAddr, (socklen_t *)&addrlen))<0)
        {
            cout << "Accept failed!!" << endl;
            exit(EXIT_FAILURE);
        } else {
            cout << "New connection!" << endl;
            pthread_create(&tid[thread_count], NULL, respond, &new_socket);
            pthread_detach(tid[thread_count]);
            pthread_mutex_lock(&mutex1);
            thread_count++;
            cout << "Number of connections:" << thread_count << endl;
            pthread_mutex_unlock(&mutex1);
        }
        while (thread_count > 99)
        {
            sleep(1);
        }
    }
    return 0;
    
}

void *respond(void *arg)
{
    int new_socket;
    char buffer[1024] = {0};
    char return_message[1024] = {0};
    int nBytes;
    char *temp, *t;
    vector<string> token;

    new_socket = *((int *)arg);
    do {
        nBytes=read(new_socket, buffer, 1024);
        buffer[nBytes] = '\0';
        if (nBytes!=0)
        {
            cout << "I received: " << buffer << endl;
            temp = strdup(buffer);
            t = strtok(temp, " ");
            
            while(t!=NULL)
            {
                token.push_back(t);
                t = strtok(NULL, " ");
            }

            printf("Tokens are:\n");
            for (auto i: token)
                cout << i << endl;
            token.clear();
            if (send(new_socket, buffer, strlen(buffer), 0) == -1)
            {
                cout << "Send failed!" << endl;
                close(new_socket);
                exit(EXIT_FAILURE);
            }
        }
        
    } while((nBytes !=0) && strcmp(buffer, "Quit") != 0);
    cout << "Client disconnected!" << endl;
    pthread_mutex_lock(&mutex1);
    thread_count --;
    pthread_mutex_unlock(&mutex1);
    close(new_socket);
    pthread_exit(NULL);
}
