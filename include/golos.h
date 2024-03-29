#ifndef GOLOS_H
#define GOLOS_H

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <miniaudio.h>

struct go_socket {
  int fd;
  struct sockaddr_in addr;
};

struct go_device_config {
  ma_format format;
  ma_uint32 channels;
  ma_uint32 sample_rate;
};

struct go_client_data_socket {
  struct go_socket client;
  struct go_socket control;
  ma_decoder decoder; // For read (File mode)
  ma_encoder encoder; // For write (Capture mode)
  double *buffer;
};

struct go_socket go_client_connect(char *address, int port, int protocol) {
  struct go_socket client;
  if ((client.fd = socket(AF_INET, protocol, 0)) == -1) {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  // int flags = fcntl(client.fd, F_GETFD);
  // if (flags == -1) {
  // }
  // flags |= O_NONBLOCK;
  // if (fcntl(client.fd, F_SETFD, flags) == -1) {
  // }
  if (setsockopt(client.fd, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int)) ==
      -1) {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(EXIT_FAILURE);
  };
  printf("Create socket\n");

  memset(&client.addr, 0, sizeof(struct sockaddr_in));
  client.addr.sin_family = AF_INET;
  if (inet_pton(AF_INET, address, &client.addr.sin_addr.s_addr) == -1) {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  client.addr.sin_port = htons(port);

  if (connect(client.fd, (struct sockaddr *)&client.addr,
              sizeof(struct sockaddr_in)) != 0) {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  printf("Connect by socket\n");

  return client;
}

/**
 * @brief Setup socket for accepting new clients.
 *
 * @param[in] port Server port in host order.
 */
struct go_socket go_server_init_udp(uint16_t port) {
  struct go_socket server;
  if ((server.fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(EXIT_FAILURE);
  };

  if (setsockopt(server.fd, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int)) ==
      -1) {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(EXIT_FAILURE);
  };
  printf("Socket created\n");

  memset(&server.addr, 0, sizeof(struct sockaddr_in));
  server.addr.sin_family = AF_INET;
  server.addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server.addr.sin_port = htons(port);
  if (bind(server.fd, (struct sockaddr *)&server.addr,
           sizeof(struct sockaddr_in)) == -1) {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(EXIT_FAILURE);
  };
  printf("Socket binded to %d\n", port);
  return server;
}

/**
 * @brief Setup socket for accepting new clients.
 *
 * @param[in] port Server port in host order.
 * @param[in] backlog Server number of outstanding connections in the socket's
 * listen queue.
 */
struct go_socket go_server_init_tcp(uint16_t port, int backlog) {
  struct go_socket server;
  if ((server.fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(EXIT_FAILURE);
  };
  // int flags = fcntl(server.fd, F_GETFD);
  // if (flags == -1) {
  // }
  // flags |= O_NONBLOCK;
  // if (fcntl(server.fd, F_SETFD, flags) == -1) {
  // }
  if (setsockopt(server.fd, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int)) ==
      -1) {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(EXIT_FAILURE);
  };
  printf("Socket created\n");

  memset(&server.addr, 0, sizeof(struct sockaddr_in));
  server.addr.sin_family = AF_INET;
  server.addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server.addr.sin_port = htons(port);
  if (bind(server.fd, (struct sockaddr *)&server.addr,
           sizeof(struct sockaddr_in)) == -1) {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(EXIT_FAILURE);
  };
  printf("Socket binded to %d\n", port);
  if (listen(server.fd, backlog) == -1) {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(EXIT_FAILURE);
  };
  printf("Listening...\n");
  return server;
}

#endif // GOLOS_H
