#include "golos.h"

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MINIAUDIO_IMPLEMENTATION
#include <external/miniaudio.h>

struct go_client_state {
  ma_decoder decoder;
  struct go_socket client;
};

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput,
                   ma_uint32 frameCount) {
  struct go_client_state *state = (struct go_client_state *)pDevice->pUserData;
  if (state == NULL) {
    return;
  }

  double *buffer = calloc(frameCount, sizeof(double));
  if (buffer == NULL) {
    fprintf(stderr, "%s\n", strerror(errno));
    close(state->client.fd);
    exit(EXIT_FAILURE);
  }
  if (ma_decoder_read_pcm_frames(&state->decoder, buffer, frameCount, NULL) !=
      MA_SUCCESS) {
    fprintf(stderr, "%s\n", strerror(errno));
    close(state->client.fd);
    exit(EXIT_FAILURE);
  }
  if (send(state->client.fd, buffer, sizeof(double) * frameCount, 0) == -1) {
    fprintf(stderr, "%s\n", strerror(errno));
    close(state->client.fd);
    exit(EXIT_FAILURE);
  };
  printf("Sended %d frames\n", frameCount);
  free(buffer);
  (void)pInput;
  (void)pOutput;
}

int main(int argc, char *argv[]) {
  uint16_t port = 0;
  char *address = NULL;
  char *file = NULL;
  int opt = -1;

  while ((opt = getopt(argc, argv, "a:p:f:")) != -1) {
    switch (opt) {
    case 'a': {
      address = optarg;
      break;
    }
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
    case 'f': {
      file = optarg;
      break;
    }
    default: {
      fprintf(stderr, "Usage: %s [-a address] [-p port] [-f file]\n", argv[0]);
      exit(EXIT_FAILURE);
    }
    }
  }

  if (address == NULL) {
    fprintf(stderr, "The -a option is required.\n");
    fprintf(stderr, "Usage: %s [-a address] [-p port] [-f file]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  if (port == 0) {
    fprintf(stderr, "The -p option is required.\n");
    fprintf(stderr, "Usage: %s [-a address] [-p port] [-f file]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  if (file == NULL) {
    fprintf(stderr, "The -f option is required.\n");
    fprintf(stderr, "Usage: %s [-a address] [-p port] [-f file]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  struct go_client_state state;

  if ((state.client.fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  printf("Create socket\n");

  memset(&state.client.addr, 0, sizeof(struct sockaddr_in));
  state.client.addr.sin_family = AF_INET;
  if (inet_pton(AF_INET, address, &state.client.addr.sin_addr.s_addr) == -1) {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  state.client.addr.sin_port = htons(port);

  if (connect(state.client.fd, (struct sockaddr *)&state.client.addr,
              sizeof(struct sockaddr_in)) != 0) {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  printf("Connect by socket\n");

  ma_result result;
  ma_device_config deviceConfig;
  ma_device device;
  struct go_device_config goDeviceConfig;

  result = ma_decoder_init_file(file, NULL, &state.decoder);
  if (result != MA_SUCCESS) {
    printf("Could not load file: %s\n", argv[1]);
    return -2;
  }
  printf("Init file\n");

  deviceConfig = ma_device_config_init(ma_device_type_playback);
  deviceConfig.playback.format = state.decoder.outputFormat;
  deviceConfig.playback.channels = state.decoder.outputChannels;
  deviceConfig.sampleRate = state.decoder.outputSampleRate;
  goDeviceConfig.format = state.decoder.outputFormat;
  goDeviceConfig.channels = state.decoder.outputChannels;
  goDeviceConfig.sample_rate = state.decoder.outputSampleRate;
  deviceConfig.dataCallback = data_callback;
  deviceConfig.pUserData = &state;

  if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
    printf("Failed to open playback device.\n");
    ma_decoder_uninit(&state.decoder);
    return -3;
  }
  printf("Init device\n");

  send(state.client.fd, &goDeviceConfig, sizeof(struct go_device_config), 0);

  if (ma_device_start(&device) != MA_SUCCESS) {
    printf("Failed to start playback device.\n");
    ma_device_uninit(&device);
    ma_decoder_uninit(&state.decoder);
    return -4;
  }
  printf("Device start\n");

  printf("Press Enter to quit...");
  getchar();

  ma_device_uninit(&device);
  ma_decoder_uninit(&state.decoder);

  close(state.client.fd);
  return EXIT_SUCCESS;
}
