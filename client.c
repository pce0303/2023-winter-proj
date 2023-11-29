
#include <arpa/inet.h>  // ntohs()
#include <stdio.h>      // printf(), perror()
#include <stdlib.h>     // atoi()
#include <string.h>     // strlen()
#include <sys/socket.h> // socket(), connect(), send(), recv()
#include <unistd.h>     // close()

#include "helper.h" // make_client_sockaddr()

#define BUFFER_LEN 1024

typedef struct {
  int sockfd;
  struct sockaddr_in addr;
} Connection;

int create_connection(Connection *conn, const char *hostname, int port) {
  // Create a socket
  conn->sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
  if (conn->sockfd == -1) {
    perror("Error creating socket");
    return -1;
  }

  // Create a sockaddr_in to specify remote host and port
  if (make_client_sockaddr(&conn->addr, hostname, port) == -1) {
    return -1;
  }

  // Connect to remote server
  if (connect(conn->sockfd, (struct sockaddr *)&conn->addr, sizeof(conn->addr)) == -1) {
    perror("Error connecting stream socket");
    return -1;
  }

  return 0;
}

int send_message(Connection *conn, const char *message) {
  const size_t message_len = strlen(message);
  if (message_len > BUFFER_LEN) {
    perror("Error: Message exceeds maximum length\n");
    return -1;
  }

  // Send message to remote server
  ssize_t sent = 0;
  do {
    const ssize_t n = send(conn->sockfd, message + sent, message_len - sent, 0);
    if (n == -1) {
      perror("Error sending on stream socket");
      return -1;
    }
    sent += n;
  } while (sent < message_len);

  return 0;
}

void close_connection(Connection *conn) {
  close(conn->sockfd);
}

int main(int argc, const char **argv) {
  // Parse command line arguments
  if (argc != 4) {
    printf("Usage: ./client hostname port_num message\n");
    return 1;
  }
  const char *hostname = argv[1];
  const int port = atoi(argv[2]);
  const char *message = argv[3];

  printf("Sending message %s to %s:%d\n", message, hostname, port);

  Connection conn;
  if (create_connection(&conn, hostname, port) == -1) {
    return 1;
  }

  if (send_message(&conn, message) == -1) {
    close_connection(&conn);
    return 1;
  }

  close_connection(&conn);

  return 0;
}