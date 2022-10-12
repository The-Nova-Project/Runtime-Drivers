#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 8080
int flag = 0;
uint32_t transact_value;

uint32_t receiver_thread(void* arg)
{

    int alwaystrue = 1;
    uint32_t received_value;
    
    while (alwaystrue) {
        if(flag == 1){
            received_value = transact_value;
            break;
        }
    }

    return received_value;
            
        

    // return NULL;
}

void* transmitter_thread(char* trans){
    int i;
    uint32_t tr;
    // for(i = 0; i < 50; i++){
    //    printf("%d \n", i);
    // }
    flag = 1;
    // convert trans from char* to uint32_t
    // sprintf(tr, "%s", trans);
    transact_value = trans;
    return NULL;
}


int main(int argc, char const* argv[])
{
    pthread_t transmitter;
    // pthread_create(&receiver, NULL, receiver_thread, NULL);
    // pthread_create(&transmitter, NULL, transmitter, NULL);

    // pthread_join(receiver, NULL);
    // pthread_join(transmitter, NULL);
    // transmitter(NULL);

    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    uint32_t hello2;
    char* hello = "Hello from server";
    

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR , &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    int alwaystrue = 1;
    while (alwaystrue) {
        

        valread = read(new_socket, buffer, 1024);
        // define send_msg variable


        char* lines_msg = "--------------------------------------------\n";
        char* send_msg = "Transmitting Message\n";
        send(new_socket, lines_msg, strlen(lines_msg), 0);
        send(new_socket, send_msg, strlen(send_msg), 0);
        send(new_socket, buffer, strlen(buffer), 0);
        send(new_socket, lines_msg, strlen(lines_msg), 0);
        pthread_create(&transmitter, NULL, transmitter_thread, buffer);
        pthread_join(transmitter, NULL);
        hello2 = receiver_thread(NULL);
        sprintf(hello, "%d", hello2);
        // hello = (char *)hello2;


        



        char* recv_msg = "Received Message\n";
        send(new_socket, lines_msg, strlen(lines_msg), 0);
        send(new_socket, recv_msg, strlen(recv_msg), 0);
        printf("%s", hello);
        // send(new_socket, hello, strlen(hello), 0);
        send(new_socket, lines_msg, strlen(lines_msg), 0);
    
    }

    //closing the connected socket
    close(new_socket);

    //closing the listening socket
    shutdown(server_fd, SHUT_RDWR);

    return 0;
}
