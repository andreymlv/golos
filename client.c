#include "util.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput,
                   ma_uint32 frameCount) {
  MA_ASSERT(pDevice->capture.format == pDevice->playback.format);
  MA_ASSERT(pDevice->capture.channels == pDevice->playback.channels);

  /* In this example the format and channel count are the same for both input
   * and output which means we can just memcpy(). */
  MA_COPY_MEMORY(pOutput, pInput,
                 frameCount *
                     ma_get_bytes_per_frame(pDevice->capture.format,
                                            pDevice->capture.channels));
}

int main(int argc, char *argv[]) {
  ma_result result;
  ma_device_config deviceConfig;
  ma_device device;

  deviceConfig = ma_device_config_init(ma_device_type_duplex);
  deviceConfig.capture.pDeviceID = NULL;
  deviceConfig.capture.format = ma_format_unknown;
  deviceConfig.capture.channels = 0;
  deviceConfig.capture.shareMode = ma_share_mode_shared;
  deviceConfig.playback.pDeviceID = NULL;
  deviceConfig.playback.format = ma_format_unknown;
  deviceConfig.playback.channels = 0;
  deviceConfig.dataCallback = data_callback;
  result = ma_device_init(NULL, &deviceConfig, &device);
  if (result != MA_SUCCESS) {
    die("Can't initialize device\n");
  }
  ma_device_start(&device);
  printf("Press Enter to quit...\n");
  getchar();
  ma_device_uninit(&device);
  return 0;
}
