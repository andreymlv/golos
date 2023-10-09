#include "golos.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#define BUFFER_SIZE 10240

struct user_data {
  int data_connection;
  double *buffer;
};

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput,
                   ma_uint32 frameCount) {
  struct user_data *data = (struct user_data *)pDevice->pUserData;
  if (data == NULL) {
    return;
  }
  size_t total = 0;
  size_t to_receive = frameCount * sizeof(double);
  while (total < to_receive) {
    ssize_t bytes = recv(data->data_connection, data->buffer + total,
                         to_receive - total, 0);
    if (bytes == -1) {
      fprintf(stderr, "%s\n", strerror(errno));
      close(data->data_connection);
    }
    total += bytes;
  }
  memcpy(pOutput, data->buffer, total);
  printf("%zu\n", total);
  (void)pInput;
}

struct options {
  uint16_t port;
};

struct options parse(int argc, char *argv[]) {
  struct options res = {0};
  int opt = -1;

  while ((opt = getopt(argc, argv, "p:")) != -1) {
    switch (opt) {
    case 'p': {
      char *endptr;
      res.port = strtoull(optarg, &endptr, 10);
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

  if (res.port == 0) {
    fprintf(stderr, "The -p option is required.\n");
    fprintf(stderr, "Usage: %s [-p port]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  return res;
}

void handle_client(int connection, int data_connection) {
  struct go_device_config goDeviceConfig = {0};
  if (recv(connection, &goDeviceConfig, sizeof(struct go_device_config), 0) ==
      -1) {
    fprintf(stderr, "%s\n", strerror(errno));
    close(connection);
    close(data_connection);
    exit(EXIT_FAILURE);
  }
  printf("Received config: %d, %d, %d\n", goDeviceConfig.format,
         goDeviceConfig.channels, goDeviceConfig.sample_rate);

  struct user_data user_data;
  user_data.data_connection = data_connection;
  user_data.buffer = calloc(BUFFER_SIZE, sizeof(double));
  ma_device_config deviceConfig;
  ma_device device;
  deviceConfig = ma_device_config_init(ma_device_type_playback);
  deviceConfig.playback.format = goDeviceConfig.format;
  deviceConfig.playback.channels = goDeviceConfig.channels;
  deviceConfig.sampleRate = goDeviceConfig.sample_rate;
  deviceConfig.dataCallback = data_callback;
  deviceConfig.pUserData = &user_data;

  if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
    printf("Failed to open playback device.\n");
    close(connection);
    close(data_connection);
    exit(EXIT_FAILURE);
  }

  if (ma_device_start(&device) != MA_SUCCESS) {
    printf("Failed to start playback device.\n");
    close(connection);
    close(data_connection);
    ma_device_uninit(&device);
    exit(EXIT_FAILURE);
  }

  // TODO(andreymlv): poll recv and timeout on end of track
  char buffer[256];
  if (recv(connection, buffer, 255, 0) == -1) {
    fprintf(stderr, "%s\n", strerror(errno));
    close(connection);
    close(data_connection);
    exit(EXIT_FAILURE);
  }

  ma_device_uninit(&device);
  free(user_data.buffer);
}

void sigchld_handler(int s) {
  (void)s;
  // waitpid() might overwrite errno, so we save and restore it:
  int saved_errno = errno;

  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;

  errno = saved_errno;
}

int main(int argc, char *argv[]) {
  struct options opt = parse(argc, argv);
  struct go_socket control = go_server_init_tcp(opt.port, 64);
  struct go_socket data = go_server_init_tcp(opt.port + 1, 64);
  struct sockaddr_in client_addr;
  struct sigaction sa;
  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  socklen_t client_len = sizeof(struct sockaddr_in);
  while (true) {
    int connection =
        accept(control.fd, (struct sockaddr *)&client_addr, &client_len);
    if (connection == -1) {
      fprintf(stderr, "%s\n", strerror(errno));
      continue;
    }
    int data_connection =
        accept(data.fd, (struct sockaddr *)&client_addr, &client_len);
    if (data_connection == -1) {
      fprintf(stderr, "%s\n", strerror(errno));
      close(connection);
      continue;
    }

    pid_t pid = fork();
    if (pid == -1) {
      fprintf(stderr, "%s\n", strerror(errno));
      close(connection);
      close(data_connection);
      close(control.fd);
      close(data.fd);
      exit(EXIT_FAILURE);
    } else if (pid == 0) {
      close(control.fd);
      close(data.fd);

      char client_ip[INET_ADDRSTRLEN];
      if (inet_ntop(AF_INET, &client_addr.sin_addr, client_ip,
                    INET_ADDRSTRLEN) == NULL) {
        fprintf(stderr, "%s\n", strerror(errno));
        close(connection);
        close(data_connection);
        exit(EXIT_FAILURE);
      };
      printf("New connection: %s\n", client_ip);

      handle_client(connection, data_connection);

      close(connection);
      close(data_connection);
      printf("Close connection\n");
      exit(EXIT_SUCCESS);
    } else {
      close(connection);
      close(data_connection);
    }
  }

  close(data.fd);
  close(control.fd);

  exit(EXIT_SUCCESS);
}
