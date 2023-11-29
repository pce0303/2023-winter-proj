#include <arpa/inet.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
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
  struct sockaddr_in addr;
  unsigned char key[EVP_MAX_KEY_LENGTH];  // 암호화 키
} Connection;

// 데이터를 암호화하는 함수
int encrypt_data(const unsigned char *plaintext, int plaintext_len, unsigned char *key,
                 unsigned char *iv, unsigned char *ciphertext) {
  EVP_CIPHER_CTX *ctx;
  int len;
  int ciphertext_len;

  // 컨텍스트 생성
  if(!(ctx = EVP_CIPHER_CTX_new())) return -1;

  // 암호화 초기화
  if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) return -1;

  // 데이터 암호화
  if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)) return -1;
  ciphertext_len = len;

  // 암호화 완료
  if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) return -1;
  ciphertext_len += len;

  // 컨텍스트 해제
  EVP_CIPHER_CTX_free(ctx);

  return ciphertext_len;
}

int create_connection(Connection *conn, const char *hostname, int port, const char *key) {
  // 소켓 생성
  conn->sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
  if (conn->sockfd == -1) {
    perror("Error creating socket");
    return -1;
  }

  // 키 복사
  strncpy((char *)conn->key, key, sizeof(conn->key));

  // 서버 주소 설정
  if (make_client_sockaddr(&conn->addr, hostname, port) == -1) {
    return -1;
  }

  // 서버에 연결
  if (connect(conn->sockfd, (struct sockaddr *)&conn->addr, sizeof(conn->addr)) == -1) {
    perror("Error connecting stream socket");
    return -1;
  }

  return 0;
}

int send_message(Connection *conn, const char *message) {
  unsigned char iv[IV_LEN];
  unsigned char encrypted_message[BUFFER_LEN];
  unsigned char buffer[BUFFER_LEN + IV_LEN];
  int encrypted_len;

  // 무작위 IV 생성
  if (!RAND_bytes(iv, sizeof(iv))) {
    perror("Error generating IV");
    return -1;
  }

  // 메시지 암호화
  encrypted_len = encrypt_data((unsigned char *)message, strlen(message), conn->key, iv, encrypted_message);
  if (encrypted_len == -1) {
    perror("Error encrypting message");
    return -1;
  }

  // IV와 암호화된 메시지 결합
  memcpy(buffer, iv, IV_LEN);
  memcpy(buffer + IV_LEN, encrypted_message, encrypted_len);

  // 메시지 전송
  if (send(conn->sockfd, buffer, IV_LEN + encrypted_len, 0) == -1) {
    perror("Error sending on stream socket");
    return -1;
  }

  return 0;
}

void close_connection(Connection *conn) {
  close(conn->sockfd);
}

int main(int argc, const char **argv) {
  if (argc != 5) {
    printf("Usage: ./client hostname port_num encryption_key message\n");
    return 1;
  }

  const char *hostname = argv[1];
  const int port = atoi(argv[2]);
  const char *key = argv[3];
  const char *message = argv[4];

  Connection conn;
  if (create_connection(&conn, hostname, port, key) == -1) {
    return 1;
  }

  if (send_message(&conn, message) == -1) {
    close_connection(&conn);
    return 1;
  }

  close_connection(&conn);

  return 0;
}
