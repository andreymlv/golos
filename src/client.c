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
#include <miniaudio.h>

#define BUFFER_SIZE 10240

void data_callback_file(ma_device *pDevice, void *pOutput, const void *pInput,
                        ma_uint32 frameCount) {
  struct go_client_data_socket *state =
      (struct go_client_data_socket *)pDevice->pUserData;
  if (state == NULL) {
    return;
  }
  if (ma_decoder_read_pcm_frames(&state->decoder, state->buffer, frameCount,
                                 NULL) != MA_SUCCESS) {
    fprintf(stderr, "%s\n", strerror(errno));
    close(state->client.fd);
  }
  if (send(state->client.fd, state->buffer, sizeof(double) * frameCount, 0) ==
      -1) {
    fprintf(stderr, "%s\n", strerror(errno));
    close(state->client.fd);
  }
  (void)pInput;
  (void)pOutput;
}

ma_result encoder_on_write(ma_encoder *pEncoder, const void *pInput,
                           size_t bytesToWrite, size_t *pBytesWritten) {
  struct go_client_data_socket *state =
      (struct go_client_data_socket *)pEncoder->pUserData;
  if (state == NULL) {
    return 1;
  }
  size_t total = 0;
  memcpy(state->buffer, pInput, bytesToWrite);
  while (total < bytesToWrite) {
    int bytes =
        send(state->client.fd, state->buffer + total, bytesToWrite - total, 0);
    if (bytes == -1) {
      fprintf(stderr, "%s\n", strerror(errno));
      close(state->client.fd);
    }
    total += bytes;
  }
  printf("%zu\n", bytesToWrite);
  printf("%zu\n", total);
  (void)pBytesWritten;
  return 0;
}

ma_result encoder_on_seek(ma_encoder *pEncoder, ma_int64 offset,
                          ma_seek_origin origin) {
  (void)pEncoder;
  (void)offset;
  (void)origin;
  return 0;
}

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput,
                   ma_uint32 frameCount) {
  struct go_client_data_socket *state =
      (struct go_client_data_socket *)pDevice->pUserData;
  if (state == NULL) {
    return;
  }
  ma_encoder_write_pcm_frames(&state->encoder, pInput, frameCount, NULL);
  (void)pOutput;
}

struct options {
  uint16_t port;
  char *address;
  char *file;
};

struct options parse(int argc, char *argv[]) {
  struct options res = {0, NULL, NULL};
  int opt = -1;

  while ((opt = getopt(argc, argv, "a:p:f:")) != -1) {
    switch (opt) {
    case 'a': {
      res.address = optarg;
      break;
    }
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
    case 'f': {
      res.file = optarg;
      break;
    }
    default: {
      fprintf(stderr, "Usage: %s -a <address> -p <port> [-f <file>]\n",
              argv[0]);
      exit(EXIT_FAILURE);
    }
    }
  }

  if (res.address == NULL) {
    fprintf(stderr, "The -a option is required.\n");
    fprintf(stderr, "Usage: %s -a <address> -p <port> [-f <file>]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  if (res.port == 0) {
    fprintf(stderr, "The -p option is required.\n");
    fprintf(stderr, "Usage: %s -a <address> -p <port> [-f <file>]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  if (res.file == NULL) {
    puts("Capture mode");
  }

  return res;
}

int main(int argc, char *argv[]) {
  struct options opt = parse(argc, argv);

  struct go_client_data_socket state;
  state.control = go_client_connect(opt.address, opt.port, SOCK_STREAM);
  state.client = go_client_connect(opt.address, opt.port + 1, SOCK_STREAM);
  state.buffer = calloc(BUFFER_SIZE, sizeof(double));

  ma_result result;
  ma_device_config deviceConfig;
  ma_device device;
  struct go_device_config goDeviceConfig = {0};

  if (opt.file == NULL) {
    ma_encoder_config config =
        ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, 2, 44100);
    result = ma_encoder_init(encoder_on_write, encoder_on_seek, &state, &config,
                             &state.encoder);
    if (result != MA_SUCCESS) {
      return result;
    }
    state.encoder.pUserData = &state;
    deviceConfig = ma_device_config_init(ma_device_type_capture);
    deviceConfig.capture.pDeviceID = NULL;
    deviceConfig.capture.format = state.encoder.config.format;
    deviceConfig.capture.channels = state.encoder.config.channels;
    deviceConfig.sampleRate = state.encoder.config.sampleRate;
    deviceConfig.capture.shareMode = ma_share_mode_shared;
    deviceConfig.dataCallback = data_callback;
    deviceConfig.pUserData = &state;

    goDeviceConfig.format = state.encoder.config.format;
    goDeviceConfig.channels = state.encoder.config.channels;
    goDeviceConfig.sample_rate = state.encoder.config.sampleRate;

    result = ma_device_init(NULL, &deviceConfig, &device);
    if (result != MA_SUCCESS) {
      return result;
    }
    puts("Init device");
    if (ma_device_start(&device) != MA_SUCCESS) {
      puts("Failed to start playback device.");
      ma_device_uninit(&device);
      return -4;
    }
    puts("Device start");
  } else {
    result = ma_decoder_init_file(opt.file, NULL, &state.decoder);
    if (result != MA_SUCCESS) {
      printf("Could not load file: %s\n", argv[1]);
      return -2;
    }
    puts("Init file");

    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = state.decoder.outputFormat;
    deviceConfig.playback.channels = state.decoder.outputChannels;
    deviceConfig.sampleRate = state.decoder.outputSampleRate;
    goDeviceConfig.format = state.decoder.outputFormat;
    goDeviceConfig.channels = state.decoder.outputChannels;
    goDeviceConfig.sample_rate = state.decoder.outputSampleRate;
    deviceConfig.dataCallback = data_callback_file;
    deviceConfig.pUserData = &state;

    ma_uint64 pcms;
    ma_decoder_get_length_in_pcm_frames(&state.decoder, &pcms);
    printf("%f seconds\n", (double)pcms / (double)goDeviceConfig.sample_rate);

    if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
      printf("Failed to open playback device.\n");
      ma_decoder_uninit(&state.decoder);
      return -3;
    }
    puts("Init device");
    if (ma_device_start(&device) != MA_SUCCESS) {
      puts("Failed to start playback device.");
      ma_device_uninit(&device);
      ma_decoder_uninit(&state.decoder);
      return -4;
    }
    puts("Device start");
  }

  if (send(state.control.fd, &goDeviceConfig, sizeof(struct go_device_config),
           0) == -1) {
    fprintf(stderr, "%s\n", strerror(errno));
    ma_device_uninit(&device);
    ma_decoder_uninit(&state.decoder);
    return -4;
  }
  printf("Send config: %d, %d, %d\n", goDeviceConfig.format,
         goDeviceConfig.channels, goDeviceConfig.sample_rate);

  puts("Type /help for list of commands");
  char *line = NULL;
  size_t size = 0;
  while (true) {
    // TODO(andreymlv): poll stdin and recv
    printf("> ");
    if (getline(&line, &size, stdin) == -1) {
      fprintf(stderr, "%s\n", strerror(errno));
      break;
    }
    if (strcmp(line, "/close\n") == 0) {
      send(state.control.fd, "close", 6, 0);
      break;
    }
    if (strcmp(line, "/help\n") == 0) {
      printf("Available commands:\n");
      printf("\t\t/help\t\t - show this message\n");
      printf("\t\t/close\t\t - close connection\n");
      continue;
    }
  }
  free(line);

  ma_device_uninit(&device);
  ma_decoder_uninit(&state.decoder);
  close(state.client.fd);
  close(state.control.fd);
  return EXIT_SUCCESS;
}
