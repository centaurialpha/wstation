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

#define WIFI_SSID CONFIG_WEATHER_WIFI_SSID
#define WIFI_PASS CONFIG_WEATHER_WIFI_PASS
#define HTTP_URL CONFIG_WEATHER_URL
#define HTTP_HOST CONFIG_WEATHER_HOST

static const dht_sensor_type_t sensor_type = DHT_TYPE_DHT11;
static const gpio_num_t dht_gpio = 4;

static void wifi_init_sta(void)
{

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

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
    ESP_ERROR_CHECK(esp_wifi_connect());
    ESP_LOGI("WIFI", "wifi_init_sta finished");
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

void publish_data(void *args)
{
    int16_t temperature_C = 0;
    int16_t humidity = 0;
    char data_buf[40];

    esp_http_client_config_t http_cfg = {
        .url = HTTP_URL,
        .host = HTTP_HOST,
        .method = HTTP_METHOD_POST,
    };

    esp_http_client_handle_t client = esp_http_client_init(&http_cfg);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    while( 1 )
    {
        if( dht_read_data(sensor_type, dht_gpio, &humidity, &temperature_C) == ESP_OK )
        {
            temperature_C /= 10;
            humidity /= 10;
            sprintf(data_buf, "{\"temp_c\": %d, \"humidity\": %d}", temperature_C, humidity);
            esp_err_t err_post = esp_http_client_set_post_field(client, data_buf, strlen(data_buf));
            ESP_ERROR_CHECK(err_post);
            esp_err_t err = esp_http_client_perform(client);

            if( err == ESP_OK )
            {
                ESP_LOGI("HTTP", "OK!");
            } else {
                ESP_LOGE("HTTP", "NOOOOOO: %s", esp_err_to_name(err));
            }

            printf("Tem C: %dC - Hum: %d%%\n", temperature_C, humidity);
        } else {
            printf("ERROR\n");
        }
        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
}


void app_main()
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_sta();

    xTaskCreate(blink, "BLINK", 2048, NULL, 1, NULL);
    xTaskCreate(publish_data, "PUBLISHER", 4096, NULL, 5, NULL);
}
