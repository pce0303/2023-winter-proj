#include <arpa/inet.h>
#include <openssl/evp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "helper.h"

#define BUFFER_LEN 1024
#define IV_LEN 16  // AES-256-CBC의 IV 길이

typedef struct {
  int sockfd;
  int port;
  int queue;
  unsigned char key[EVP_MAX_KEY_LENGTH];  // 암호화 키
} Server;

// 데이터를 복호화하는 함수
int decrypt_data(const unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
                 unsigned char *iv, unsigned char *plaintext) {
  EVP_CIPHER_CTX *ctx;
  int len;
  int plaintext_len;

  // 컨텍스트 생성
  if(!(ctx = EVP_CIPHER_CTX_new())) return -1;

  // 복호화 초기화
  if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) return -1;

  // 데이터 복호화
  if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) return -1;
  plaintext_len = len;

  // 복호화 완료
  if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) return -1;
  plaintext_len += len;

  // 컨텍스트 해제
  EVP_CIPHER_CTX_free(ctx);

  return plaintext_len;
}

// 클라이언트와의 연결을 처리하는 함수
// 클라이언트와의 연결을 처리하는 함수
int handle_connection(int connectionfd, unsigned char *key) {
    printf("New connection %d\n", connectionfd);

    unsigned char buffer[BUFFER_LEN + IV_LEN]; // IV를 포함하는 버퍼
    unsigned char decrypted_message[BUFFER_LEN];
    unsigned char iv[IV_LEN];
    memset(buffer, 0, sizeof(buffer));
    memset(decrypted_message, 0, sizeof(decrypted_message));
    memset(iv, 0, sizeof(iv));

    ssize_t recvd = 0;
    ssize_t reval;
    do {
        reval = recv(connectionfd, buffer + recvd, BUFFER_LEN + IV_LEN - recvd, 0);
        if (reval == -1) {
            perror("Error reading stream message");
            return -1;
        }
        recvd += reval;
    } while (reval > 0);

    // IV 추출
    memcpy(iv, buffer, IV_LEN);

    // 데이터 복호화
    int decrypted_len = decrypt_data(buffer + IV_LEN, recvd - IV_LEN, key, iv, decrypted_message);
    if (decrypted_len == -1) {
        printf("Error decrypting message from client %d\n", connectionfd);
        // 복호화에 실패했지만 연결은 유지합니다.
    } else {
        printf("Client %d says: '%s'\n", connectionfd, decrypted_message);
    }

    return 0; // 연결 종료 대신 0을 반환하여 함수를 정상적으로 종료합니다.
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

    if (handle_connection(connectionfd, server.key) == -1) {
      return -1;
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Usage: ./server port_num encryption_key\n");
    return 1;
  }

  Server server;
  server.port = atoi(argv[1]);
  server.queue = 10;

  // 암호화 키 설정
  strncpy((char *)server.key, argv[2], sizeof(server.key));

  if (run_server(server) == -1) {
    return 1;
  }

  return 0;
}
