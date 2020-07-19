#include <esp_http_client.h>
#include <esp_log.h>
#include <string.h>
#include <cJSON.h>

#include "google_api.h"

static char *http_buffer = NULL;
static esp_err_t http_status;
static esp_err_t (*_http_cb)(esp_err_t status, cJSON *json);

static esp_err_t parse_http_buffer(esp_http_client_event_t *evt)
{
  ESP_LOGD(TAG, "Http buffer:\n\t%s", http_buffer);
  cJSON *json = cJSON_Parse(http_buffer);
  if (json == NULL)
  {
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr)
    {
      ESP_LOGE(TAG, "Error while parsing to JSON:\n\t%s", error_ptr);
    }
    return _http_cb(ESP_FAIL, NULL);
  }
  return _http_cb(ESP_OK, json);
}

static esp_err_t gcp_http_event_handler(esp_http_client_event_t *evt)
{
  switch (evt->event_id)
  {
  case HTTP_EVENT_ERROR:
    ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
    http_status = _http_cb(ESP_FAIL, NULL);
    break;
  case HTTP_EVENT_ON_CONNECTED:
    break;
  case HTTP_EVENT_HEADER_SENT:
    break;
  case HTTP_EVENT_ON_HEADER:
    break;
  case HTTP_EVENT_ON_DATA:
    if (esp_http_client_is_chunked_response(evt->client))
    {
      ESP_LOGD(TAG, "Handling chunked message");
    }
    else
    {
      ESP_LOGD(TAG, "Handling unchunked message");
    }
    ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
    if (http_buffer)
    {
      const size_t buffer_size = strlen(http_buffer);
      const size_t new_size = buffer_size + evt->data_len + 1;
      char *new_buffer = realloc(http_buffer, new_size);
      if (!new_buffer)
      {
        ESP_LOGE(TAG, "Failed to resize http_buffer with realloc!");
        free(http_buffer);
        return _http_cb(ESP_FAIL, NULL);
      }
      http_buffer = new_buffer;
      memcpy(http_buffer + buffer_size, (char *)evt->data, evt->data_len);
      http_buffer[new_size - 1] = 0;
    }
    else
    {
      http_buffer = malloc(evt->data_len + 1);
      memcpy(http_buffer, (char *)evt->data, evt->data_len);
      http_buffer[evt->data_len] = 0;
    }
    ESP_LOGD(TAG, "Chunk of http buffer:\n\t%s", http_buffer);
    break;
  case HTTP_EVENT_ON_FINISH:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
    http_status = parse_http_buffer(evt);
    break;
  case HTTP_EVENT_DISCONNECTED:
    ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
    free(http_buffer);
    http_buffer = NULL;
    break;
  }
  return http_status;
}

http_event_handle_cb *gcp_build_event_handle(gcp_http_response_handler http_cb)
{
  _http_cb = http_cb;
  http_status = ESP_FAIL;
  return gcp_http_event_handler;
}

void gcp_clean_access_token()
{
  if (ACCESS_TOKEN)
  {
    free(ACCESS_TOKEN);
    ESP_LOGW(TAG, "ACCESS_TOKEN cleaned!");
  }
}