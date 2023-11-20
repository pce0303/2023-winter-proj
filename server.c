#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys.socket.h>
#include <unistd.h>

#include "helper.h"

//#define PORT 9090;
static const size_t BUFFER_LEN 1024;

int handle_connection(connetctionfd){
    printf("New connection %d/n", connectionfd);
    
    char buffer[BUFFER_LEN + 1];
    memset(buffer, 0, sizeof(buffer));
    
    //call recv() to consume all the data the client sends
    ssize_t recvd = 0;
    ssize_t rval;
    do {
        rval = recv(connectionfd, buffer + recvd, BUFFER_LEN - recvd, 0);
        if (reval == -1){
            perror("error reading stream message");
            return -1;
        }
        recvd += reval;
    } while (reval > 0); //recv() returns 0 when client close
    
    //print message
    printf("client %d says '%s'/n", connectionfd, buffer);
    
    //close connection
    close(connectionfd);
    
    return 0;
}

int run_server(int port, int queue){
    //create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sockfd == -1) {
        perror("error opening stream socket");
        return -1;
    }
    
    //set the "reuse port" socket option
    int yesval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yesval, sizeof(yesval)) == -1){
        perror("error setting socket options");
        return -1;
    }
    
    //create socket_addr
    struct sockaddr_in addr;
    if(make_server_socketaddr(&addr, port) == -1){
        return -1;
    }
    
    //bind to the port
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1){
        perror("error binding stream socket");
        return -1;
    }
    
    //detect which port was chosen
    port = get_port_number(sockfd);
    printf("server listening on port %d/n", port);
    
    //listening for incoming connections
    if (listen(sockfd, queue) == -1){
        perror("error listening");
        return -1;
    }
    
    //accepting connection one by one forever
    while (1) {
        int connectionfd = accept(sockfd, NULL, NULL);
        if (connectionfd == -1){
            perror("error accepting connections");
            return -1;
        }
        
        if(handle_connection(connectionfd) == -1){
            return -1;
        }
    }
    
}

int main(int argc, char *argv[]){
    if(argc != 2){
        return 1;
    }
    const int port  = atoi(argv[1]);
    
    if(run_server(port, 10) == -1){
        return 1;
    }
    return 0;
}
