/* Ignore this file for now */
#ifndef CONFIG_GCP_DEVICE_CODE

#include <esp_http_client.h>
#include <esp_log.h>
#include <string.h>
#include <cJSON.h>

#include "google_api.h"

void gcp_auth_get_device_code()
{
  ESP_LOGI(TAG, "Get device token...");

  char *BODY = "scope=https://www.googleapis.com/auth/devstorage.read_write&client_id=%s";
  int body_len = strlen(BODY) + strlen(CONFIG_GCP_CLIENT_ID) + 1;
  char *post_data = malloc(body_len);
  snprintf(post_data, body_len, BODY, CONFIG_GCP_CLIENT_ID);

  esp_http_client_config_t config = {
      .url = "https://accounts.google.com/o/oauth2/device/code",
      // .event_handler = _http_event_handler,
      .method = HTTP_METHOD_POST};
  esp_http_client_handle_t http_client = esp_http_client_init(&config);
  esp_http_client_set_post_field(http_client, post_data, strlen(post_data));
  ESP_LOGD(TAG, "Len: %i, Body:\n\t%s", strlen(post_data), post_data);
  esp_http_client_set_header(http_client, "Content-Type", "application/x-www-form-urlencoded");

  esp_err_t err = esp_http_client_perform(http_client);

  ESP_LOGI(TAG, "Status = %d, content_length = %d",
           esp_http_client_get_status_code(http_client),
           esp_http_client_get_content_length(http_client));
  esp_http_client_cleanup(http_client);

  free(post_data);
}

#endif