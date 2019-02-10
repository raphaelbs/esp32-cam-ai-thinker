#include <esp_http_client.h>
#include <esp_log.h>
#include <string.h>
#include <cJSON.h>

#include "google_api.h"

static const char *TAG = "gcp_storage_insert_object";
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

  char *post_url = malloc(256);
  sprintf(post_url, "https://www.googleapis.com/upload/storage/v1/b/%s/o?uploadType=media&name=%s", CONFIG_GCP_BUCKET, pic_name);

  char *authorization = malloc(256);
  sprintf(authorization, "Bearer %s", ACCESS_TOKEN);

  esp_http_client_config_t config = {
      .url = post_url,
      .event_handler = gcp_build_event_handle(http_handler_cb),
      .method = HTTP_METHOD_POST};
  esp_http_client_handle_t http_client = esp_http_client_init(&config);
  esp_http_client_set_post_field(http_client, binary, binary_size);

  esp_http_client_set_header(http_client, "Content-Type", "image/jpg");
  esp_http_client_set_header(http_client, "Authorization", authorization);

  esp_err_t err = esp_http_client_perform(http_client);
  if (err == ESP_OK)
  {
    ESP_LOGW(TAG, "Status = %d", esp_http_client_get_status_code(http_client));
  }
  esp_http_client_cleanup(http_client);

  free(post_url);
  free(authorization);

  return http_status;
}