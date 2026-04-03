
#include <esp_log.h>
#include <esp_event.h>
#include <esp_http_server.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <string.h>

#include "Network.h"

static const char *LOGTAG __attribute__((unused)) = "NET";

static esp_netif_t *soft_ap = NULL;

static void
wifi_event_handler( void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data )
{
}

esp_err_t
networkInit()
{
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	soft_ap = esp_netif_create_default_wifi_ap();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, NULL));

	char *ssid = "Waveshare";

	wifi_config_t wifi_config = {
		.ap = {
			.password = "12345678",
			.max_connection = 8,
			.authmode = WIFI_AUTH_WPA2_PSK,
			.pmf_cfg = {
					.required = true,
			},
		},
	};
	int l = strlen(ssid);
	memcpy(wifi_config.ap.ssid, ssid, l);
	wifi_config.ap.ssid_len = l;
	if (wifi_config.ap.password[0] == 0) {
		wifi_config.ap.authmode = WIFI_AUTH_OPEN;
	}

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

#if 0
	esp_netif_config_t conf = ESP_NETIF_DEFAULT_WIFI_AP();
	soft_ap = esp_netif_new(&conf);
#endif

	return ESP_OK;
}

static const char *page1 =
"<html>"
"<head>"
"</head>"
"<body>"
"Well ok then"
"</body>"
"</html>";

esp_err_t
my_uri_handler( httpd_req_t *req )
{
	httpd_resp_send(req, page1, HTTPD_RESP_USE_STRLEN);
	return ESP_OK;
}

static const httpd_uri_t my_uri = {
	.uri      = "/",
	.method   = HTTP_GET,
	.handler  = my_uri_handler,
	.user_ctx = "xxx"
};

esp_err_t
networkStartHttpd()
{
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	httpd_handle_t server = NULL;
	config.uri_match_fn = httpd_uri_match_wildcard;
	ESP_LOGI(LOGTAG, "max_uri_handlers = %d", config.max_uri_handlers);
	ESP_ERROR_CHECK(httpd_start(&server, &config));
	ESP_ERROR_CHECK(httpd_register_uri_handler(server, &my_uri));
	return ESP_OK;
}

