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

#define MAXBUFLEN 4096

static int init_socket(char **port) {

  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int rv;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE; // use my IP

  if ((rv = getaddrinfo(NULL, *port, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // loop through all the results and bind to the first we can
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("listener: socket");
      continue;
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("listener: bind");
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "listener: failed to bind socket\n");
    return 2;
  }

  freeaddrinfo(servinfo);

  return sockfd;
}

static void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "usage: %s port\n", argv[0]);
    exit(1);
  }

  int sockfd = init_socket(&argv[1]);
  socklen_t addr_len;
  struct sockaddr_storage their_addr;
  int numbytes;
  char buf[MAXBUFLEN];
  char s[INET6_ADDRSTRLEN];

  printf("listener: waiting to recvfrom...\n");

  if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0, NULL, NULL)) == -1) {
    perror("recvfrom");
    exit(1);
  }
  buf[numbytes] = '\0';
  const char *client_address =
      inet_ntop(their_addr.ss_family,
                get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
  printf("listener: got packet from %s\n", client_address);
  printf("listener: packet is %d bytes long\n", numbytes);
  printf("listener: packet contains \"%s\"\n", buf);

  close(sockfd);

  return 0;
}
