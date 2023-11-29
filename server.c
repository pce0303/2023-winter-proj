
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "helper.h"

#define BUFFER_LEN 1024

typedef struct {
  int sockfd;
  int port;
  int queue;
} Server;

// 클라이언트와의 연결을 처리하는 함수
int handle_connection(int connectionfd) {
  printf("New connection %d\n", connectionfd);

  char buffer[BUFFER_LEN];
  memset(buffer, 0, sizeof(buffer));

  ssize_t recvd = 0;
  ssize_t reval;
  do {
    reval = recv(connectionfd, buffer + recvd, BUFFER_LEN - recvd, 0);
    if (reval == -1) {
      perror("Error reading stream message");
      return -1;
    }
    recvd += reval;
  } while (reval > 0);

  printf("Client %d says: '%s'\n", connectionfd, buffer);

  close(connectionfd);

  return 0;
}

// 서버를 실행하는 함수
int run_server(Server server) {
  // 소켓 생성
  server.sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
  if (server.sockfd == -1) {
    perror("Error opening stream socket");
    return -1;
  }

  int yesval = 1;
  if (setsockopt(server.sockfd, SOL_SOCKET, SO_REUSEADDR, &yesval, sizeof(yesval)) == -1) {
    perror("Error setting socket options");
    return -1;
  }

  struct sockaddr_in addr;
  if (make_server_socketaddr(&addr, server.port) == -1) {
    return -1;
  }

  if (bind(server.sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("Error binding stream socket");
    return -1;
  }

  server.port = get_port_number(server.sockfd);
  printf("Server listening on port %d\n", server.port);

  if (listen(server.sockfd, server.queue) == -1) {
    perror("Error listening");
    return -1;
  }

  while (1) {
    int connectionfd = accept(server.sockfd, NULL, NULL);
    if (connectionfd == -1) {
      perror("Error accepting connections");
      return -1;
    }

    if (handle_connection(connectionfd) == -1) {
      return -1;
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    printf("Usage: ./server port_num\n");
    return 1;
  }

  Server server;
  server.port = atoi(argv[1]);
  server.queue = 10;

  if (run_server(server) == -1) {
    return 1;
  }

  return 0;
}