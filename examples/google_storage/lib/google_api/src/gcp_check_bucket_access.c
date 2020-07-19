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
    ESP_LOGE(TAG, "Could not access the bucket!");
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "Bucket read!");
  return ESP_OK;
}

esp_err_t gcp_check_bucket_access()
{
  ESP_LOGI(TAG, "Checking bucket access [%s]...", CONFIG_GCP_BUCKET);

  char *URL = "https://www.googleapis.com/storage/v1/b?project=%s&maxResults=1&prefix=%s";
  int post_url_len = strlen(URL) + strlen(CONFIG_GCP_PROJECT) + strlen(CONFIG_GCP_BUCKET) + 1;
  char *post_url = malloc(post_url_len);
  snprintf(post_url, post_url_len, URL, CONFIG_GCP_PROJECT, CONFIG_GCP_BUCKET);

  char *AUTH_VALUE = "Bearer %s";
  int auth_len = strlen(AUTH_VALUE) + strlen(ACCESS_TOKEN) + 1;
  char *authorization = malloc(auth_len);
  snprintf(authorization, auth_len, AUTH_VALUE, ACCESS_TOKEN);

  esp_http_client_config_t config = {
      .url = post_url,
      .event_handler = gcp_build_event_handle(http_handler_cb),
      .method = HTTP_METHOD_GET};
  esp_http_client_handle_t http_client = esp_http_client_init(&config);
  esp_http_client_set_header(http_client, "Authorization", authorization);
  esp_http_client_set_header(http_client, "Transfer-Encoding", "chunked");

  esp_err_t err = esp_http_client_perform(http_client);
  if (err == ESP_OK)
  {
    ESP_LOGI(TAG, "Status = %d", esp_http_client_get_status_code(http_client));
  }
  esp_http_client_cleanup(http_client);

  free(post_url);
  free(authorization);

  return http_status;
}