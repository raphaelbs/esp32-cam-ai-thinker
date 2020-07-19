#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <time.h>
#include <sys/time.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_http_client.h>
#include <string.h>
#include "lwip/err.h"
#include "lwip/apps/sntp.h"

#include "esp_camera.h"
#include "google_api.h"

#define WIFI_CONNECTED BIT0
#define SYNC_NTP BIT1
#define HAS_AUTH_TOKEN BIT2

static EventGroupHandle_t eventGroup;

static camera_config_t camera_config = {
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

static void clear_auth_token()
{
  xEventGroupClearBits(eventGroup, HAS_AUTH_TOKEN);
  ESP_LOGI(TAG, "Clear HAS_AUTH_TOKEN");
}

static void take_picture_and_upload_to_google_storage()
{
  ESP_LOGI(TAG, "Taking picture...");
  camera_fb_t *pic = esp_camera_fb_get();

  struct tm timeinfo = {0};
  time_t now = 0;
  time(&now);
  localtime_r(&now, &timeinfo);
  int pic_name_len = 76;
  char *pic_name = malloc(pic_name_len);
  snprintf(pic_name, pic_name_len,
    "%04d-%02d-%02d_%02d-%02d-%02d.jpg",
    timeinfo.tm_year + 1900,
    timeinfo.tm_mon + 1,
    timeinfo.tm_mday,
    timeinfo.tm_hour,
    timeinfo.tm_min,
    timeinfo.tm_sec
  );

  ESP_LOGI(TAG, "Picture %s taken.", pic_name);
  ESP_LOGI(TAG, "Uploading %s...", pic_name);

  esp_err_t err = gcp_storage_insert_object(pic_name, (const char *)pic->buf, pic->len);
  if (err == ESP_FAIL)
  {
    clear_auth_token(err);
  }

  free(pic_name);
}

static void initialize_sntp(void)
{
  ESP_LOGI(TAG, "Initializing SNTP");
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "pool.ntp.org");
  sntp_init();
}

static void task_gcp()
{
  heap_caps_print_heap_info(MALLOC_CAP_8BIT);
  while (1)
  {
    EventBits_t bit = xEventGroupWaitBits(eventGroup, (WIFI_CONNECTED | SYNC_NTP | HAS_AUTH_TOKEN), false, false, portMAX_DELAY);

    if (bit == WIFI_CONNECTED)
    {
      initialize_sntp();
      struct tm timeinfo = {0};
      time_t now = 0;
      int retry = 0;
      const int retry_count = 10;
      while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count)
      {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
      }
      ESP_LOGI(TAG, "Date from NTP:\t%04d-%02d-%02d_%02d-%02d-%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
      ESP_LOGI(TAG, "Set SYNC_NTP");
      xEventGroupSetBits(eventGroup, SYNC_NTP);
    }
    else if (bit == (WIFI_CONNECTED | SYNC_NTP))
    {
      if (gcp_auth_refresh_token() == ESP_OK && gcp_check_bucket_access() == ESP_OK)
      {
        ESP_LOGI(TAG, "Set HAS_AUTH_TOKEN");
        xEventGroupSetBits(eventGroup, HAS_AUTH_TOKEN);
      }
      else
      {
        clear_auth_token();
      }
    }
    else if (bit == (WIFI_CONNECTED | SYNC_NTP | HAS_AUTH_TOKEN))
    {
      take_picture_and_upload_to_google_storage();
    }

    heap_caps_print_heap_info(MALLOC_CAP_8BIT);
    vTaskDelay(20000 / portTICK_RATE_MS);
  }
}

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
  switch (event->event_id)
  {
  case SYSTEM_EVENT_STA_START:
    ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
    ESP_ERROR_CHECK(esp_wifi_connect());
    break;
  case SYSTEM_EVENT_STA_GOT_IP:
    ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
    ESP_LOGI(TAG, "Got IP: '%s'",
             ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
    xEventGroupSetBits(eventGroup, WIFI_CONNECTED);
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
    ESP_ERROR_CHECK(esp_wifi_connect());
    xEventGroupClearBits(eventGroup, WIFI_CONNECTED);
    break;
  default:
    break;
  }
  return ESP_OK;
}

static void initialise_wifi()
{
  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  wifi_config_t wifi_config = {
      .sta = {
          .ssid = CONFIG_WIFI_SSID,
          .password = CONFIG_WIFI_PASSWORD,
      },
  };
  ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
}

void app_main()
{
  esp_err_t err = nvs_flash_init();
  if (err != ESP_OK)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(err);
  }
  eventGroup = xEventGroupCreate();
  ESP_ERROR_CHECK(esp_camera_init(&camera_config));
  initialise_wifi();
  xTaskCreate(&task_gcp, "task_gcp", 12096, NULL, 10, NULL);
}
