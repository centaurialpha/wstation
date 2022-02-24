#include <string.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <dht.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "freertos/event_groups.h"

#define WIFI_SSID CONFIG_WEATHER_WIFI_SSID
#define WIFI_PASS CONFIG_WEATHER_WIFI_PASS
#define HTTP_URL CONFIG_WEATHER_URL
#define HTTP_HOST CONFIG_WEATHER_HOST
#define PUSH_DATA_PERIOD_S CONFIG_WEATHER_PERIOD_S

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const dht_sensor_type_t sensor_type = DHT_TYPE_DHT11;
static const gpio_num_t dht_gpio = 4;

static EventGroupHandle_t s_wifi_event_group;

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *data)
{
    if( event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START ) {
        ESP_LOGI("WIFI", "STA_START");
        esp_wifi_connect();
    } else if( event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED ){
        ESP_LOGV("WIFI", "Disconnected, attemping...");
        esp_wifi_connect();
    } else if( event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP ) {
        ESP_LOGI("WIFI", "GOT IP");
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init_sta(void)
{

    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS
        },
    };

    ESP_LOGI("WIFI", "Connecting...");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, "esp8266");

    ESP_LOGI("WIFI", "wifi_init_sta finished");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    if( bits & WIFI_CONNECTED_BIT ) {
        ESP_LOGI("WIFI", "Connected to %s", WIFI_SSID);
    } else if( bits & WIFI_FAIL_BIT ) {
        ESP_LOGE("WIFI", "Not connected");
    } else {
        ESP_LOGE("WIFI", "Unexpected event");
    }

    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    vEventGroupDelete(s_wifi_event_group);
}

void configure_led()
{
    gpio_config_t gpio_cfg;
    gpio_cfg.pin_bit_mask = (1ULL << 2);
    gpio_cfg.mode = GPIO_MODE_OUTPUT;
    gpio_cfg.pull_down_en = 0;
    gpio_cfg.pull_up_en = 0;
    gpio_cfg.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&gpio_cfg);
}

void blink(void *args)
{

    configure_led();

    while( 1 )
    {
        gpio_set_level(2, 0);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        gpio_set_level(2, 1);
        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
}

esp_err_t send_post(char *data_buf)
{

    esp_err_t ret_val = ESP_OK;

    esp_http_client_config_t http_cfg = {
        .url = HTTP_URL,
        .port = 8000,
        .host = HTTP_HOST,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 3000,
        .transport_type = HTTP_TRANSPORT_OVER_TCP,
    };

    esp_http_client_handle_t client = esp_http_client_init(&http_cfg);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    esp_err_t err_post = esp_http_client_set_post_field(client, data_buf, strlen(data_buf));
    ESP_ERROR_CHECK(err_post);
    esp_err_t err = esp_http_client_perform(client);

    if( err == ESP_OK )
    {
        ESP_LOGI("HTTP", "OK!");
    } else {
        ESP_LOGE("HTTP", "NOOOOOO: %s", esp_err_to_name(err));
        ret_val = ESP_FAIL;
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    return ret_val;
}

void publish_data(void *args)
{
    int16_t temp_C = 0;
    int16_t humidity = 0;
    uint8_t sensor_state = 1;

    char data_buf[60];


    while( 1 )
    {
        if( dht_read_data(sensor_type, dht_gpio, &humidity, &temp_C) == ESP_OK )
        {
            temp_C /= 10;
            humidity /= 10;
            sensor_state = 1;

        } else {
            printf("ERROR\n");
            sensor_state = 0;
        }
        sprintf(data_buf, "{\"tempc\":%d,\"humidity\":%d,\"s_state\":%d}", temp_C, humidity, sensor_state);
        ESP_ERROR_CHECK(send_post(data_buf));

        vTaskDelay((PUSH_DATA_PERIOD_S * 1000) / portTICK_PERIOD_MS);
    }

}

void app_main()
{
    wifi_init_sta();

    xTaskCreate(blink, "BLINK", 2048, NULL, 1, NULL);
    xTaskCreate(publish_data, "PUBLISHER", 4096, NULL, 5, NULL);
}
