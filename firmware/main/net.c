#include <string.h>
#include "esp_netif.h"
#include "esp_wifi.h"
//#include "esp_http_client.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "net.h"

#define WIFI_SSID       CONFIG_WIFI_SSID
#define WIFI_PASSWORD   CONFIG_WIFI_PASSWORD

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define TAG "NET"

static EventGroupHandle_t s_wifi_event_group;


static void on_wifi_disconnected(void *arg, esp_event_base_t event_base, int32_t event_id, void *data)
{
    //ESP_LOGW(TAG, "WiFi disconnected, trying to reconnect...");
    ESP_LOGW(TAG, "WiFi disconnected");
    //esp_err_t err = esp_wifi_connect();
    //if( err == ESP_ERR_WIFI_NOT_STARTED )
    //    return;
    //ESP_ERROR_CHECK(err);
}

static void on_got_ip(void *arg, esp_event_base_t event_base, int32_t event_id, void *data)
{
    ESP_LOGI(TAG, "Got ip!");
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
}

void disconnect_wifi( void )
{
    ESP_LOGI(TAG, "Disconnecting wifi...");
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &on_wifi_disconnected));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip));

    esp_err_t err = esp_wifi_stop();
    if (err == ESP_ERR_WIFI_NOT_INIT) {
        return;
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_wifi_deinit());

    vEventGroupDelete(s_wifi_event_group);
}

void connect_wifi( void )
{
    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnected, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip, NULL));

    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD
        },
    };

    ESP_LOGI(TAG, "Connecting to %s...", WIFI_SSID);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, "wstation");
    ESP_LOGI(TAG, "wifi_init_sta finished.");

    esp_wifi_connect();

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    if( bits & WIFI_CONNECTED_BIT ) {
        ESP_LOGI(TAG, "Connected to %s", WIFI_SSID);
    } else if( bits & WIFI_FAIL_BIT ) {
        ESP_LOGW(TAG, "Could not connect to wifi");
    } else {
        ESP_LOGE(TAG, "Unexpected event");
    }
}

//esp_err_t send_post(char *data)
//{
//    esp_err_t ret_val = ESP_OK;
//
//    esp_http_client_config_t cfg = {
//        .url = HTTP_URL,
//        .port = 8000,
//        .host = HTTP_HOST,
//        .method = HTTP_METHOD_POST,
//        .transport_type = HTTP_TRANSPORT_OVER_TCP,
//    };
//
//    esp_http_client_handle_t client = esp_http_client_init(&cfg);
//    esp_http_client_set_header(client, "Content-Type", "application/json");
//
//    esp_err_t err_post = esp_http_client_set_post_field(client, data, strlen(data));
//    ESP_ERROR_CHECK(err_post);
//
//    esp_err_t err = esp_http_client_perform(client);
//
//    if( err == ESP_OK )
//    {
//        ESP_LOGI(TAG, "OK!");
//    } else {
//        ESP_LOGE(TAG, "Noooo: %s", esp_err_to_name(err));
//        ret_val = ESP_FAIL;
//    }
//
//    esp_http_client_close(client);
//    esp_http_client_cleanup(client);
//
//    return ret_val;
//}

