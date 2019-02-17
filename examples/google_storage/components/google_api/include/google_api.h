/**
 * This component follows Google's guidelines to utilize
 * GCP services though OAuth 2.0 authentication.
 * 
 * You can get more information at:
 * https://developers.google.com/identity/protocols/OAuth2ForDevices
 */
#ifndef __GCP_GOOGLE_API__
#define __GCP_GOOGLE_API__

#include <cJSON.h>
#include <esp_http_client.h>

char *ACCESS_TOKEN;
const static char *TAG = "example_google_storage";

void gcp_clean_access_token();

typedef esp_err_t (*gcp_http_response_handler)(esp_err_t status, cJSON *json);

http_event_handle_cb *gcp_build_event_handle(gcp_http_response_handler http_cb);

#ifndef CONFIG_GCP_DEVICE_CODE
/**
 * Gets a new device code for this given device.
 * Read more at:
 * https://developers.google.com/identity/protocols/OAuth2ForDevices#step-1-request-device-and-user-codes
 */
void gcp_auth_get_device_code();
#endif

#ifndef CONFIG_GCP_REFRESH_TOKEN
/**
 * Gets a new token for this given device.
 * Read more at:
 * https://developers.google.com/identity/protocols/OAuth2ForDevices#step-4-poll-googles-authorization-server
 */
void gcp_auth_get_token();
#endif

/**
 * Refresh the access token using the refresh token.
 * Read more at:
 * https://developers.google.com/identity/protocols/OAuth2ForDevices#offline
 */
esp_err_t gcp_auth_refresh_token();

esp_err_t gcp_storage_insert_object(const char *pic_name, const char *binary, size_t binary_size);

esp_err_t gcp_check_bucket_access();

#endif