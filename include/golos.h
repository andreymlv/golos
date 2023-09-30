#ifndef GOLOS_H
#define GOLOS_H

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <external/miniaudio.h>

struct go_socket {
  int fd;
  struct sockaddr_in addr;
};

struct go_device_config {
  ma_format format;
  ma_uint32 channels;
  ma_uint32 sample_rate;
};

/**
 * @brief Setup socket for accepting new clients.
 *
 * @param[in] port Server port in host order.
 * @param[in] backlog Server number of outstanding connections in the socket's
 * listen queue.
 */
struct go_socket go_server_init(uint16_t port, int backlog) {
  struct go_socket server;
  if ((server.fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
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
  if (listen(server.fd, backlog) == -1) {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(EXIT_FAILURE);
  };
  return server;
}

#endif // GOLOS_H
