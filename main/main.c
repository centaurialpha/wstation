#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sleep.h"
#include "mqtt_client.h"

#include "sensor.h"
#include "net.h"

#define TAG "WSTATION"

#define MQTT_BROKER     CONFIG_MQTT_BROKER

RTC_DATA_ATTR static int boot_count = 0;

void publish_dht_data(esp_mqtt_client_handle_t client)
{
    dht_data_t data = {0};
    char temp[20];
    char humidity[20];
    char state[20];

    if( get_dht_sensor_data(&data) != ESP_OK )
    {
        printf("Error reading sensor..\n");
    }

    sprintf(temp, "%f,dht", data.temperature);
    sprintf(humidity, "%f,dht", data.humidity);
    sprintf(state, "%d,dht", data.state);

    esp_mqtt_client_publish(client, "home/temp", temp, 0, 1, 0);
    esp_mqtt_client_publish(client, "home/humidity", humidity, 0, 1, 0);
    esp_mqtt_client_publish(client, "home/state", state, 0, 1, 0);
}

void publish_bmp_data(esp_mqtt_client_handle_t client)
{

    bmp_data_t data = {0};
    char temp[20];
    char pressure[20];
    char state[20];

    ESP_ERROR_CHECK(bmp_init());

    if( get_bmp_sensor_data(&data) != ESP_OK )
    {
        printf("Error reading sensor..\n");
    }

    sprintf(temp, "%f,bmp", data.temperature);
    sprintf(pressure, "%d,bmp", data.pressure);
    sprintf(state, "%d,bmp", data.state);

    esp_mqtt_client_publish(client, "home/temp", temp, 0, 1, 0);
    esp_mqtt_client_publish(client, "home/pressure", pressure, 0, 1, 0);
    esp_mqtt_client_publish(client, "home/state", state, 0, 1, 0);
}

static void mqtt_event_handler(void *args, esp_event_base_t base, int32_t event_id, void *data)
{
    esp_mqtt_event_handle_t event = data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch( (esp_mqtt_event_id_t)event_id )
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
#ifdef CONFIG_PUBLISH_DHT_DATA
            publish_dht_data(client);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
#endif
#ifdef CONFIG_PUBLISH_BMP_DATA
            publish_bmp_data(client);
#endif
            esp_mqtt_client_disconnect(client);
            disconnect_wifi();
            vTaskDelay(1000 / portTICK_PERIOD_MS);

            ESP_LOGI(TAG, "Entering deep sleep for %d seconds", CONFIG_DEEP_SLEEP_WAKE_UP_SECS);
            esp_deep_sleep(1e6 * CONFIG_DEEP_SLEEP_WAKE_UP_SECS);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        default:
            ESP_LOGI(TAG, "Other event id: %d", event->event_id);
            break;
    }
}

void start_mqtt()
{
    esp_mqtt_client_config_t cfg = {
        .uri = CONFIG_MQTT_BROKER,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void app_main()
{
    boot_count += 1;
    ESP_LOGI(TAG, "Boot count: %d", boot_count);

    ESP_ERROR_CHECK(i2cdev_init());
    connect_wifi();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    start_mqtt();
}
