#include "golos.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput,
                   ma_uint32 frameCount) {
  int *connection_fd = (int *)pDevice->pUserData;
  if (connection_fd == NULL) {
    return;
  }
  double *buffer = calloc(frameCount, sizeof(double));
  if (buffer == NULL) {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  int read = recvfrom(*connection_fd, buffer, sizeof(double) * frameCount, 0,
                      NULL, NULL);
  if (read == -1) {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(EXIT_FAILURE);
  };
  if (read == 0) {
    exit(EXIT_SUCCESS);
  }
  memcpy(pOutput, buffer, sizeof(double) * frameCount);
  free(buffer);
  (void)pInput;
}

int main(int argc, char *argv[]) {
  uint16_t port = 0;
  int opt = -1;

  while ((opt = getopt(argc, argv, "p:")) != -1) {
    switch (opt) {
    case 'p': {
      char *endptr;
      port = strtoull(optarg, &endptr, 10);
      if (errno != 0) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(EXIT_FAILURE);
      }
      if (endptr == optarg) {
        fprintf(stderr, "No digits were found\n");
        exit(EXIT_FAILURE);
      }
      break;
    }
    default: {
      fprintf(stderr, "Usage: %s [-p port]\n", argv[0]);
      exit(EXIT_FAILURE);
    }
    }
  }

  if (port == 0) {
    fprintf(stderr, "The -p option is required.\n");
    fprintf(stderr, "Usage: %s [-p port]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  struct go_socket server = go_server_init(port, 10);
  struct go_socket control = go_server_init(port + 1, 10);

  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(struct sockaddr_in);
  while (true) {
    if (recvfrom(control.fd, NULL, 1, 0, (struct sockaddr *)&client_addr,
                 &client_len) == -1) {
      fprintf(stderr, "%s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }
    char client_ip[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN) ==
        NULL) {
      fprintf(stderr, "%s\n", strerror(errno));
      exit(EXIT_FAILURE);
    };
    printf("New connection: %s\n", client_ip);
    pid_t pid = fork();
    if (pid == -1) {
      fprintf(stderr, "%s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }
    if (pid == 0) {
      struct go_device_config goDeviceConfig;
      if (recvfrom(control.fd, &goDeviceConfig, sizeof(struct go_device_config),
                   0, NULL, NULL) == -1) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(EXIT_FAILURE);
      }
      printf("Received config\n");
      ma_device_config deviceConfig;
      ma_device device;
      deviceConfig = ma_device_config_init(ma_device_type_playback);
      deviceConfig.playback.format = goDeviceConfig.format;
      deviceConfig.playback.channels = goDeviceConfig.channels;
      deviceConfig.sampleRate = goDeviceConfig.sample_rate;
      deviceConfig.dataCallback = data_callback;
      deviceConfig.pUserData = &server.fd;

      if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
        printf("Failed to open playback device.\n");
        return -3;
      }

      if (ma_device_start(&device) != MA_SUCCESS) {
        printf("Failed to start playback device.\n");
        ma_device_uninit(&device);
        return -4;
      }

      if (recvfrom(control.fd, NULL, 1, 0, (struct sockaddr *)&client_addr,
                   &client_len) == -1) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(EXIT_FAILURE);
      }

      close(control.fd);
      close(server.fd);

      ma_device_uninit(&device);
      printf("Close connection\n");
      exit(EXIT_SUCCESS);
    }
  }

  close(server.fd);
  close(control.fd);
  exit(EXIT_SUCCESS);
}
