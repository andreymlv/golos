#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static int init_socket(char **hostname, char **port, struct addrinfo *p) {
  int sockfd;
  struct addrinfo hints, *servinfo;
  int rv;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
  hints.ai_socktype = SOCK_DGRAM;
  if ((rv = getaddrinfo(*hostname, *port, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("talker: socket");
      continue;
    }

    break;
  }
  freeaddrinfo(servinfo);
  if (p == NULL) {
    fprintf(stderr, "talker: failed to create socket\n");
    return 2;
  }
  return sockfd;
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    fprintf(stderr, "usage: %s hostname port message\n", argv[0]);
    exit(1);
  }
  struct addrinfo *p;
  int sockfd = init_socket(&argv[1], &argv[2], p);
  int numbytes;
  if ((numbytes = sendto(sockfd, argv[3], strlen(argv[3]), 0, p->ai_addr,
                         p->ai_addrlen)) == -1) {
    perror("talker: sendto");
    exit(1);
  }
  printf("talker: sent %d bytes to %s\n", numbytes, argv[1]);
  close(sockfd);
  return 0;
}
