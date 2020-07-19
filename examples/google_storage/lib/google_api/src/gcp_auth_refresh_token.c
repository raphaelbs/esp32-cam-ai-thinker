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
    ESP_LOGE(TAG, "Request FAIL!");
    return ESP_FAIL;
  }
  gcp_clean_access_token();
  cJSON *access_token = cJSON_GetObjectItem(json, "access_token");
  if (cJSON_IsString(access_token) && access_token->valuestring)
  {
    size_t token_size = strlen(access_token->valuestring);
    ACCESS_TOKEN = malloc(7 + token_size);
    memcpy(ACCESS_TOKEN, "Bearer ", 7);
    memcpy(ACCESS_TOKEN + 7, access_token->valuestring, token_size);
    ESP_LOGI(TAG, "Access_token:\t\"%s\"", access_token->valuestring);
    return ESP_OK;
  }
  ESP_LOGE(TAG, "Parse FAIL!");
  return ESP_FAIL;
}

esp_err_t gcp_auth_refresh_token()
{
  ESP_LOGI(TAG, "Refresh auth token...");

  char *BODY = "grant_type=refresh_token&client_id=%s&client_secret=%s&refresh_token=%s";
  int body_len = strlen(BODY) + strlen(CONFIG_GCP_CLIENT_ID) + strlen(CONFIG_GCP_CLIENT_SECRET) + strlen(CONFIG_GCP_REFRESH_TOKEN) + 1;
  char *post_data = malloc(body_len);
  snprintf(post_data, body_len,
          BODY,
          CONFIG_GCP_CLIENT_ID,
          CONFIG_GCP_CLIENT_SECRET,
          CONFIG_GCP_REFRESH_TOKEN);

  esp_http_client_config_t config = {
      .url = "https://www.googleapis.com/oauth2/v4/token",
      .event_handler = gcp_build_event_handle(http_handler_cb),
      .method = HTTP_METHOD_POST};
  esp_http_client_handle_t http_client = esp_http_client_init(&config);
  esp_http_client_set_post_field(http_client, post_data, strlen(post_data));
  ESP_LOGD(TAG, "Len: %i, Body:\n\t%s", strlen(post_data), post_data);
  esp_http_client_set_header(http_client, "Content-Type", "application/x-www-form-urlencoded");

  esp_err_t err = esp_http_client_perform(http_client);
  if (err == ESP_OK)
  {
    ESP_LOGI(TAG, "Status = %d", esp_http_client_get_status_code(http_client));
  }
  esp_http_client_cleanup(http_client);

  free(post_data);

  return http_status;
}