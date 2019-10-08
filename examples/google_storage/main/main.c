#include <time.h>
#include <sys/time.h>
#include <esp_system.h>
#include <sys/param.h>
#include "lwip/err.h"
#include "lwip/apps/sntp.h"

#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <nvs_flash.h>

#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>
#include <wifi_provisioning/scheme_softap.h>

#include "esp_camera.h"
#include "google_api.h"

#define WIFI_CONNECTED_EVENT BIT0
#define SYNC_NTP BIT1
#define HAS_AUTH_TOKEN BIT2

static EventGroupHandle_t eventGroup;
static EventGroupHandle_t wifi_event_group;

static camera_config_t camera_config = {
    .pin_pwdn = -1,
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
  char *pic_name = malloc(sizeof("YYYY-mm-dd_hh-MM-ss.jpg"));
  sprintf(pic_name, "%04d-%02d-%02d_%02d-%02d-%02d.jpg", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

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
    EventBits_t bit = xEventGroupWaitBits(eventGroup, (WIFI_CONNECTED_EVENT | SYNC_NTP | HAS_AUTH_TOKEN), false, false, portMAX_DELAY);
    esp_err_t err;

    if (bit == WIFI_CONNECTED_EVENT)
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
    else if (bit == (WIFI_CONNECTED_EVENT | SYNC_NTP))
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
    else if (bit == (WIFI_CONNECTED_EVENT | SYNC_NTP | HAS_AUTH_TOKEN))
    {
      take_picture_and_upload_to_google_storage();
    }

    heap_caps_print_heap_info(MALLOC_CAP_8BIT);
    vTaskDelay(10000 / portTICK_RATE_MS);
  }
}

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
  /* Pass event information to provisioning manager so that it can
    * maintain its internal state depending upon the system event */
  wifi_prov_mgr_event_handler(ctx, event);

  /* Global event handling */
  switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
      esp_wifi_connect();
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      ESP_LOGI(TAG, "Connected with IP Address:%s",
                ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
      xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_EVENT);
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
      esp_wifi_connect();
      break;
    default:
      break;
  }
  return ESP_OK;
}

static void prov_event_handler(void *user_data,
                               wifi_prov_cb_event_t event, void *event_data)
{
  switch (event) {
    case WIFI_PROV_START:
      ESP_LOGI(TAG, "Provisioning started");
      break;
    case WIFI_PROV_CRED_RECV: {
      wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
      /* If SSID length is exactly 32 bytes, null termination
        * will not be present, so explicitly obtain the length */
      size_t ssid_len = strnlen((const char *)wifi_sta_cfg->ssid, sizeof(wifi_sta_cfg->ssid));
      ESP_LOGI(TAG, "Received Wi-Fi credentials"
                "\n\tSSID     : %.*s\n\tPassword : %s",
                ssid_len, (const char *) wifi_sta_cfg->ssid,
                (const char *) wifi_sta_cfg->password);
      break;
    }
    case WIFI_PROV_CRED_FAIL: {
      wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
      ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                "\n\tPlease reset to factory and retry provisioning",
                (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                "Wi-Fi AP password incorrect" : "Wi-Fi AP not found");
      break;
    }
    case WIFI_PROV_CRED_SUCCESS:
      ESP_LOGI(TAG, "Provisioning successful");
      break;
    case WIFI_PROV_END:
      /* De-initialize manager once provisioning is finished */
      wifi_prov_mgr_deinit();
      break;
    default:
      break;
  }
}

static void initialise_wifi()
{
  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
  wifi_event_group = xEventGroupCreate();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  wifi_prov_mgr_config_t config = {
      .scheme = wifi_prov_scheme_ble,
      .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM,
      .app_event_handler = {
          .event_cb = prov_event_handler,
          .user_data = NULL
      }
  };

  ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

  bool provisioned = false;
  ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

  if (!provisioned) {
    ESP_LOGI(TAG, "Starting provisioning");

    wifi_prov_security_t security = WIFI_PROV_SECURITY_1;
    const char *pop = "rapha123";

    uint8_t custom_service_uuid[] = {
        /* LSB <---------------------------------------
          * ---------------------------------------> MSB */
        0x21, 0x43, 0x65, 0x87, 0x09, 0xba, 0xdc, 0xfe,
        0xef, 0xcd, 0xab, 0x90, 0x78, 0x56, 0x34, 0x12
    };
    wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);

    /* Start provisioning service */
    ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, pop, "RAPHA_BRANDAO", NULL));
  } else {
    ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi STA");

    /* We don't need the manager as device is already provisioned,
      * so let's release it's resources */
    wifi_prov_mgr_deinit();

    /* Start Wi-Fi station */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
  }
  xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_EVENT, false, true, portMAX_DELAY);
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
