#include "util.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#define SAMPLE_FORMAT ma_format_f32
#define CHANNEL_COUNT 2
#define SAMPLE_RATE 48000

ma_uint32 g_decoderCount;
ma_decoder *g_pDecoders;

static void usage(const char*const program) { die("usage: %s [port]\n", program); }

static ma_data_source *next_callback_tail(ma_data_source *pDataSource) {
  MA_ASSERT(g_decoderCount >
            0); /* <-- We check for this in main() so should never happen. */

  (void)pDataSource;
  /*
  This will be fired when the last item in the chain has reached the end. In
  this example we want to loop back to the start, so we need only return a
  pointer back to the head.
  */
  return &g_pDecoders[0];
}

static void data_callback(ma_device *pDevice, void *pOutput, const void *pInput,
                          ma_uint32 frameCount) {
  /*
  We can just read from the first decoder and miniaudio will resolve the chain
  for us. Note that if you want to loop the chain, like we're doing in this
  example, you need to set the `loop` parameter to false, or else only the
  current data source will be looped.
  */
  ma_data_source_read_pcm_frames(&g_pDecoders[0], pOutput, frameCount, NULL);

  /* Unused in this example. */
  (void)pDevice;
  (void)pInput;
}

int main(int argc, char *argv[]) {
  const char *port = "8888";
  ma_result result = MA_SUCCESS;
  ma_uint32 iDecoder;
  ma_decoder_config decoderConfig;
  ma_device_config deviceConfig;
  ma_device device;
  if (argc > 2) {
    usage(argv[0]);
  } else if (argc == 2) {
    port = argv[1];
  }
  g_decoderCount = argc - 1;
  g_pDecoders = (ma_decoder *)malloc(sizeof(*g_pDecoders) * g_decoderCount);

  /* In this example, all decoders need to have the same output format. */
  decoderConfig =
      ma_decoder_config_init(SAMPLE_FORMAT, CHANNEL_COUNT, SAMPLE_RATE);
  for (iDecoder = 0; iDecoder < g_decoderCount; ++iDecoder) {
    result = ma_decoder_init_file(argv[1 + iDecoder], &decoderConfig,
                                  &g_pDecoders[iDecoder]);
    if (result != MA_SUCCESS) {
      ma_uint32 iDecoder2;
      for (iDecoder2 = 0; iDecoder2 < iDecoder; ++iDecoder2) {
        ma_decoder_uninit(&g_pDecoders[iDecoder2]);
      }
      free(g_pDecoders);

      printf("Failed to load %s.\n", argv[1 + iDecoder]);
      return -1;
    }
  }
  for (iDecoder = 0; iDecoder < g_decoderCount - 1; iDecoder += 1) {
    ma_data_source_set_next(&g_pDecoders[iDecoder], &g_pDecoders[iDecoder + 1]);
  }
  ma_data_source_set_next_callback(&g_pDecoders[g_decoderCount - 1],
                                   next_callback_tail);
  deviceConfig = ma_device_config_init(ma_device_type_playback);
  deviceConfig.playback.format = SAMPLE_FORMAT;
  deviceConfig.playback.channels = CHANNEL_COUNT;
  deviceConfig.sampleRate = SAMPLE_RATE;
  deviceConfig.dataCallback = data_callback;
  deviceConfig.pUserData = NULL;

  if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
    printf("Failed to open playback device.\n");
    result = -1;
    goto done_decoders;
  }

  if (ma_device_start(&device) != MA_SUCCESS) {
    printf("Failed to start playback device.\n");
    result = -1;
    goto done;
  }

  printf("Press Enter to quit...");
  getchar();

done:
  ma_device_uninit(&device);

done_decoders:
  for (iDecoder = 0; iDecoder < g_decoderCount; ++iDecoder) {
    ma_decoder_uninit(&g_pDecoders[iDecoder]);
  }
  free(g_pDecoders);
  return 0;
}
