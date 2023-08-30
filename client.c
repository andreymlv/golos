#include "rnnoise.h"
#include <stdlib.h>
#include <string.h>
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

float *pTempIn;
float *pTempOut;
DenoiseState *st;

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput,
                   ma_uint32 frameCount) {
  // Cast to U32.
  for (ma_uint32 i = 0; i < frameCount; i += 1) {
    pTempIn[i] = (float)((ma_int16 *)pInput)[i];
  }

  rnnoise_process_frame(st, pTempOut, pTempIn);

  // Cast back to S16.
  for (ma_uint32 i = 0; i < frameCount; i += 1) {
    ((ma_int16 *)pOutput)[i] = (ma_int16)pTempOut[i];
  }
}

int main() {
  st = rnnoise_create(NULL);
  pTempIn = malloc(rnnoise_get_frame_size() * sizeof(float));
  pTempOut = malloc(rnnoise_get_frame_size() * sizeof(float));
  ma_device_config deviceConfig;
  deviceConfig = ma_device_config_init(ma_device_type_duplex);
  deviceConfig.capture.pDeviceID = NULL;
  deviceConfig.capture.format = ma_format_s16;
  deviceConfig.capture.channels = 1;
  deviceConfig.capture.shareMode = ma_share_mode_shared;

  deviceConfig.playback.pDeviceID = NULL;
  deviceConfig.playback.format = ma_format_s16;
  deviceConfig.playback.channels = 1;
  deviceConfig.dataCallback = data_callback;

  deviceConfig.periodSizeInFrames = rnnoise_get_frame_size();

  ma_device device;
  ma_result result;

  result = ma_device_init(NULL, &deviceConfig, &device);
  if (result != MA_SUCCESS) {
    return result;
  }

  ma_device_start(&device);

  printf("Press Enter to quit...\n");
  getchar();

  ma_device_uninit(&device);
  free(pTempOut);
  free(pTempIn);
  rnnoise_destroy(st);
  return 0;
}
