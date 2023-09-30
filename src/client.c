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

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput,
                   ma_uint32 frameCount) {
  struct go_client_data_socket *state =
      (struct go_client_data_socket *)pDevice->pUserData;
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
  int sended = send(state->client.fd, buffer, sizeof(double) * frameCount, 0);
  if (sended == -1) {
    fprintf(stderr, "%s\n", strerror(errno));
    close(state->client.fd);
    exit(EXIT_FAILURE);
  };
  printf("Sended %d frames with %d bytes\n", frameCount, sended);
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

  struct go_client_data_socket state;
  state.client = go_client_connect(address, port);
  state.control = go_client_connect(address, port + 1);

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

  if (send(state.client.fd, &goDeviceConfig, sizeof(struct go_device_config),
           0) == -1) {
    fprintf(stderr, "%s\n", strerror(errno));
    ma_device_uninit(&device);
    ma_decoder_uninit(&state.decoder);
    return -4;
  }

  if (ma_device_start(&device) != MA_SUCCESS) {
    printf("Failed to start playback device.\n");
    ma_device_uninit(&device);
    ma_decoder_uninit(&state.decoder);
    return -4;
  }
  printf("Device start\n");

  puts("Press enter to stop");
  getchar();
  send(state.control.fd, NULL, 1, 0);

  ma_device_uninit(&device);
  ma_decoder_uninit(&state.decoder);
  close(state.client.fd);
  close(state.control.fd);
  return EXIT_SUCCESS;
}
