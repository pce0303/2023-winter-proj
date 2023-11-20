#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> //시스템에서 사용하는 자료형들에 대한 정보
#include <sys/socket.h> //소켓의 종류
#include <netinet/in.h>  //소켓 주소 구조체 포함
#include <netdb.h> //클라이언트와 서버 정보 구조체 포함
#include <arpa/inet.h> //주소 변환 기능
#include <unistd.h>

int make_server_socketaddr(struct sockaddr_in *addr, int port){
    addr->sin_family = AF_INET; //ipv4 internet protocol
    addr->sin_addr.s_addr = INADDR_ANY; //32bit ip address
    addr->sin_port = htons(port); //port number
    
    return 0;
}

int get_port_number(int sockfd){
    struct sockaddr_in addr;
    socklen_t length = sizeof(addr);
    
    if(getsockname(sockfd, (struct sockaddr *) &addr, &length) == -1){
        perror("error getting port of socket");
        return -1;
    }
    
    //use ntohs to convert from network byte to host byte
    return ntohs(addr.sin_port);
}
