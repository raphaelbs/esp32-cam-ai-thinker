
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_camera.h"

static const char *TAG = "example_change_detection";
QueueHandle_t queue;

static camera_config_t detection_config = {
    .pin_pwdn = CONFIG_PWDN,
    .pin_reset = CONFIG_RESET,
    .pin_xclk = CONFIG_XCLK,
    .pin_sscb_sda = CONFIG_SDA,
    .pin_sscb_scl = CONFIG_SCL,

    .pin_d7 = CONFIG_D7,
    .pin_d6 = CONFIG_D6,
    .pin_d5 = CONFIG_D5,
    .pin_d4 = CONFIG_D4,
    .pin_d3 = CONFIG_D3,
    .pin_d2 = CONFIG_D2,
    .pin_d1 = CONFIG_D1,
    .pin_d0 = CONFIG_D0,
    .pin_vsync = CONFIG_VSYNC,
    .pin_href = CONFIG_HREF,
    .pin_pclk = CONFIG_PCLK,

    //XCLK 20MHz or 10MHz
    .xclk_freq_hz = CONFIG_XCLK_FREQ,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_GRAYSCALE, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_QQVGA,       //QQVGA-UXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 12, //0-63 lower number means higher quality
    .fb_count = 2       //if more than one, i2s runs in continuous mode. Use only with JPEG
};
static camera_config_t photo_config = {
    .pin_pwdn = CONFIG_PWDN,
    .pin_reset = CONFIG_RESET,
    .pin_xclk = CONFIG_XCLK,
    .pin_sscb_sda = CONFIG_SDA,
    .pin_sscb_scl = CONFIG_SCL,

    .pin_d7 = CONFIG_D7,
    .pin_d6 = CONFIG_D6,
    .pin_d5 = CONFIG_D5,
    .pin_d4 = CONFIG_D4,
    .pin_d3 = CONFIG_D3,
    .pin_d2 = CONFIG_D2,
    .pin_d1 = CONFIG_D1,
    .pin_d0 = CONFIG_D0,
    .pin_vsync = CONFIG_VSYNC,
    .pin_href = CONFIG_HREF,
    .pin_pclk = CONFIG_PCLK,

    //XCLK 20MHz or 10MHz
    .xclk_freq_hz = CONFIG_XCLK_FREQ,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_UXGA,   //QQVGA-UXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 12, //0-63 lower number means higher quality
    .fb_count = 1       //if more than one, i2s runs in continuous mode. Use only with JPEG
};

typedef struct
{
  uint8_t *buf;
  size_t len;
} picture_t;

static void consume_image()
{
  while (1)
  {
    picture_t *picture;
    if (xQueueReceive(queue, &picture, (TickType_t)5))
    {
      ESP_LOGW(TAG, "Consume image: [%i] bytes", picture->len);
      free(picture->buf);
    }
    vTaskDelay(5000 / portTICK_RATE_MS);
  }
}

static void change_detector()
{
  esp_camera_init(&detection_config);
  int64_t timestamp;
  uint8_t avg;
  uint8_t prevAvg = 0;

  while (1)
  {
    if (uxQueueSpacesAvailable(queue) == 0)
    {
      ESP_LOGE(TAG, "No point in take more pics, queue is full. Waiting 2s");
      vTaskDelay(2000 / portTICK_RATE_MS);
      continue;
    }
    timestamp = esp_timer_get_time();
    camera_fb_t *pic = esp_camera_fb_get();

    avg = pic->buf[0];
    for (int i = 1; i < pic->len / 2; i++)
    {
      avg += (pic->buf[i] - avg) / i;
    }
    esp_camera_fb_return(pic);
    ESP_LOGD(TAG, "Detecting time: %lli, Avg: %i", (esp_timer_get_time() - timestamp), avg);

    if ((avg >= prevAvg && avg - prevAvg > CONFIG_MAX_ACCEPTED_AVG_DIFF) || (prevAvg > avg && prevAvg - avg > CONFIG_MAX_ACCEPTED_AVG_DIFF))
    {
      ESP_LOGI(TAG, "Motion detected: %i", avg);
      esp_camera_deinit();
      ESP_ERROR_CHECK(esp_camera_init(&photo_config));

      pic = esp_camera_fb_get();

      picture_t *picture = malloc(sizeof(picture_t));
      if (picture == NULL)
      {
        ESP_LOGE(TAG, "Error while malloc picture.");
      }
      else
      {
        uint8_t *buf = malloc(pic->len);
        if (buf == NULL)
        {
          ESP_LOGE(TAG, "Error while malloc picture buff.");
        }
        else
        {
          for (int i = 0; i < pic->len; i++)
          {
            buf[i] = pic->buf[i];
          }
          picture->buf = buf;
          picture->len = pic->len;

          ESP_LOGW(TAG, "Add image [%i]", picture->len);
          xQueueSend(queue, &picture, portMAX_DELAY);
        }
      }

      esp_camera_deinit();
      ESP_ERROR_CHECK(esp_camera_init(&detection_config));

      ESP_LOGW(TAG, "Picture time: %lli", (esp_timer_get_time() - timestamp));
    }
    prevAvg = avg;
  }
}

void app_main()
{
  queue = xQueueCreate(CONFIG_PICTURE_QUEUE_SIZE, sizeof(picture_t *));
  if (queue == NULL)
  {
    ESP_LOGE(TAG, "Error creating the queue");
    ESP_ERROR_CHECK(ESP_FAIL);
  }

  xTaskCreatePinnedToCore(change_detector, "change_detector", 8192, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(consume_image, "consume_image", 8192, NULL, 1, NULL, 1);
}
