#include <stdbool.h>
#include "i2s_example_pins.h"
#include "freertos/queue.h"

/* Set 1 to allocate rx & tx channels in duplex mode on a same I2S controller, they will share the BCLK and WS signal
 * Set 0 to allocate rx & tx channels in simplex mode, these two channels will be totally separated,
 * Specifically, due to the hardware limitation, the simplex rx & tx channels can't be registered on the same controllers on ESP32 and ESP32-S2,
 * and ESP32-S2 has only one I2S controller, so it can't allocate two simplex channels */
#define EXAMPLE_I2S_DUPLEX_MODE         CONFIG_USE_DUPLEX

#define EXAMPLE_STD_BCLK_IO1        EXAMPLE_I2S_BCLK_IO1      // I2S bit clock io number
#define EXAMPLE_STD_WS_IO1          EXAMPLE_I2S_WS_IO1      // I2S word select io number
#define EXAMPLE_STD_DOUT_IO1        EXAMPLE_I2S_DOUT_IO1     // I2S data out io number
#define EXAMPLE_STD_DIN_IO1         EXAMPLE_I2S_DIN_IO1     // I2S data in io number
#if !EXAMPLE_I2S_DUPLEX_MODE
#define EXAMPLE_STD_BCLK_IO2        EXAMPLE_I2S_BCLK_IO2     // I2S bit clock io number
#define EXAMPLE_STD_WS_IO2          EXAMPLE_I2S_WS_IO2     // I2S word select io number
#define EXAMPLE_STD_DOUT_IO2        EXAMPLE_I2S_DOUT_IO2     // I2S data out io number
#define EXAMPLE_STD_DIN_IO2         EXAMPLE_I2S_DIN_IO2     // I2S data in io number
#endif

#define NUM_SAMPLES             2048
#define AUDIO_BUFF_SIZE               8 * NUM_SAMPLES
#define PROCESSED_BUFF_SIZE              2 * NUM_SAMPLES

typedef struct Audio_Buffer {
    uint8_t data[PROCESSED_BUFF_SIZE];
    size_t size;
} processed_audio_t;

extern QueueHandle_t xAudioQueue; // Declare the queue handle for audio data
extern bool isQueueReady;

void i2s_example_read_task(void *args);

void i2s_example_write_task(void *args);

#if EXAMPLE_I2S_DUPLEX_MODE
void i2s_example_init_std_duplex(void);
#else
void i2s_example_init_std_simplex(void);
#endif

void raw_audio_process(const int32_t *input, processed_audio_t *output, size_t bytes_to_process);

