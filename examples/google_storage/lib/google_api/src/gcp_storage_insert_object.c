#include <esp_http_client.h>
#include <esp_log.h>
#include <string.h>
#include <cJSON.h>

#include "google_api.h"

static esp_err_t http_status;

static esp_err_t http_handler_cb(esp_err_t status, cJSON *json)
{
  http_status = status;
  if (status == ESP_FAIL)
  {
    ESP_LOGE(TAG, "Could not upload image!");
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "Image uploaded!");
  return ESP_OK;
}

esp_err_t gcp_storage_insert_object(const char *pic_name, const char *binary, size_t binary_size)
{
  ESP_LOGI(TAG, "Insert object [%s] in bucket [%s]...", pic_name, CONFIG_GCP_BUCKET);

  const char *URL = "https://www.googleapis.com/upload/storage/v1/b/%s/o?uploadType=media&name=%s";
  int post_url_len = strlen(URL) + strlen(CONFIG_GCP_BUCKET) + strlen(pic_name) + 1;
  char *post_url = malloc(post_url_len);
  snprintf(post_url, post_url_len, URL, CONFIG_GCP_BUCKET, pic_name);

  esp_http_client_config_t config = {
      .url = post_url,
      .event_handler = gcp_build_event_handle(http_handler_cb),
      .method = HTTP_METHOD_POST};
  esp_http_client_handle_t http_client = esp_http_client_init(&config);
  esp_http_client_set_post_field(http_client, binary, binary_size);

  esp_http_client_set_header(http_client, "Content-Type", "image/jpg");
  esp_http_client_set_header(http_client, "Authorization", ACCESS_TOKEN);

  esp_err_t err = esp_http_client_perform(http_client);
  if (err == ESP_OK)
  {
    ESP_LOGI(TAG, "Status = %d", esp_http_client_get_status_code(http_client));
  }
  esp_http_client_cleanup(http_client);

  free(post_url);

  return http_status;
}