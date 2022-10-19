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
pthread_t transmitter;
int alwaystrue = 1;
int talnet_socket;

uint32_t receiver_thread(void* arg)
{

    int alwaystrue = 1;
    uint32_t received_value;

    
    while (alwaystrue) {
       //-----------------------step1 read from driver-----------------------
       //-----------------------step2 send to telnet socket------------------
    }

    return received_value;
            
        

    // return NULL;
}

void* transmitter_thread(void* trans){
    int  valread;
    char buffer[1024] = {0};

     while (alwaystrue) {
        

        valread = read(talnet_socket, buffer, 1024);

        //-----------------------here call write value for driver-----------------------
        send(talnet_socket, buffer, valread, 0);
        // uint32_t value = receiver_thread(NULL);
        char convert[32];
        // sprintf(convert, "%u", value);
        // printf("writing %s to driver" , buffer );
     }



    return NULL;
}


int main(int argc, char const* argv[])
{
    pthread_t transmitter;

    int server_fd, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    
    

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
    if ((talnet_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    int alwaystrue = 1;

        pthread_create(&transmitter, NULL, transmitter_thread, NULL);
        //-----------------------call receiver_thread function-----------------------
        pthread_join(transmitter, NULL);




    //closing the connected socket
    //-----------------------uncomment these when needed-----------------------
	// close(talnet_socket);
    // close(server_fd);
        

    return 0;

    }
