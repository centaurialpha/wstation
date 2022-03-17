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

typedef struct sensor_data_t {
    char *dht_temp;
    char *dht_humidity;
    char *bmp_temp;
    char *bmp_pressure;
} sensor_data_t;

//void publish_dht_data(esp_mqtt_client_handle_t client)
//{
//    dht_data_t data = {0};
//    char temp[20];
//    char humidity[20];
//    char state[20];
//
//    if( get_dht_sensor_data(&data) != ESP_OK )
//    {
//        printf("Error reading sensor..\n");
//    }
//
//    sprintf(temp, "%f,dht", data.temperature);
//    sprintf(humidity, "%f,dht", data.humidity);
//    sprintf(state, "%d,dht", data.state);
//
//    esp_mqtt_client_publish(client, "home/temp", temp, 0, 1, 0);
//    esp_mqtt_client_publish(client, "home/humidity", humidity, 0, 1, 0);
//    esp_mqtt_client_publish(client, "home/state", state, 0, 1, 0);
//}
//
//void publish_bmp_data(esp_mqtt_client_handle_t client)
//{
//
//    bmp_data_t data = {0};
//    char temp[20];
//    char pressure[20];
//    char state[20];
//
//    ESP_ERROR_CHECK(bmp_init());
//
//    if( get_bmp_sensor_data(&data) != ESP_OK )
//    {
//        printf("Error reading sensor..\n");
//    }
//
//    sprintf(temp, "%f,bmp", data.temperature);
//    sprintf(pressure, "%d,bmp", data.pressure);
//    sprintf(state, "%d,bmp", data.state);
//
//    esp_mqtt_client_publish(client, "home/temp", temp, 0, 1, 0);
//    esp_mqtt_client_publish(client, "home/pressure", pressure, 0, 1, 0);
//    esp_mqtt_client_publish(client, "home/state", state, 0, 1, 0);
//}

static void mqtt_event_handler(void *args, esp_event_base_t base, int32_t event_id, void *data)
{
    esp_mqtt_event_handle_t event = data;
    esp_mqtt_client_handle_t client = event->client;
    sensor_data_t sensor_data = (sensor_data_t *)args;
    //int msg_id;

    switch( (esp_mqtt_event_id_t)event_id )
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            printf("%s\n", sensor_data.dht_temp);
#ifdef CONFIG_PUBLISH_DHT_DATA
            //publish_dht_data(client);
            esp_mqtt_client_publish(client, "home/temp", sensor_data.dht_temp, 0, 1, 0);
            esp_mqtt_client_publish(client, "home/humidity", sensor_data.dht_humidity, 0, 1, 0);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
#endif
#ifdef CONFIG_PUBLISH_BMP_DATA
            //publish_bmp_data(client);
            esp_mqtt_client_publish(client, "home/temp", sensor_data.bmp_temp, 0, 1, 0);
            esp_mqtt_client_publish(client, "home/pressure", sensor_data.bmp_pressure, 0, 1, 0);
#endif
            esp_mqtt_client_disconnect(client);
            disconnect_wifi();
            vTaskDelay(1000 / portTICK_PERIOD_MS);

            ESP_LOGI(TAG, "Entering deep sleep for %d seconds", CONFIG_DEEP_SLEEP_WAKE_UP_SECS);
            //esp_deep_sleep(1e6 * CONFIG_DEEP_SLEEP_WAKE_UP_SECS);
            esp_deep_sleep(1e6 * 10);
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            break;
        default:
            ESP_LOGI(TAG, "Other event id: %d", event->event_id);
            break;
    }
}

void start_mqtt(sensor_data_t *data)
{
    esp_mqtt_client_config_t cfg = {
        .uri = CONFIG_MQTT_BROKER,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, data);
    esp_mqtt_client_start(client);
}

void start()
{
    //dht_data_t dht_data;
    //bmp_data_t bmp_data;
    sensor_data_t *all_data = {0};


    dht_data_t dht_data = {
        .temperature = 10.5,
        .humidity = 50,
    };
    bmp_data_t bmp_data = {
        .temperature = 11.4,
        .pressure = 1234,
    };

    size_t nbytes = snprintf(NULL, 0, "%.1f,dht", dht_data.temperature) + 1;
    all_data->dht_temp = (char *)malloc(nbytes);
    snprintf(all_data->dht_temp, nbytes, "%.1f,dht", dht_data.temperature);

    size_t nbytes2 = snprintf(NULL, 0, "%.1f,dht", dht_data.humidity) + 1;
    all_data->dht_humidity = malloc(nbytes2);
    snprintf(all_data->dht_humidity, nbytes2, "%.1f,dht", dht_data.humidity);

    size_t nbytes3 = snprintf(NULL, 0, "%.1f,bmp", bmp_data.temperature) + 1;
    all_data->bmp_temp = malloc(nbytes3);
    snprintf(all_data->bmp_temp, nbytes3, "%.1f,bmp", bmp_data.temperature);

    size_t nbytes4 = snprintf(NULL, 0, "%d,bmp", bmp_data.pressure) + 1;
    all_data->bmp_pressure = malloc(nbytes4);
    snprintf(all_data->bmp_pressure, nbytes4, "%d,bmp", bmp_data.pressure);
    //if( get_dht_sensor_data(&dht_data) != ESP_OK )
    //{
    //    printf("Error reading DHT sensor...\n");
    //}
    //sprintf(all_data.dht_temp, "%.1f,dht", dht_data.temperature);
    //sprintf(all_data.dht_humidity, "%.1f,dht", dht_data.humidity);

    //if( get_bmp_sensor_data(&bmp_data) != ESP_OK )
    //{
    //    printf("Error reading BMP sensor..\n");
    //}

    //sprintf(all_data.bmp_temp, "%.1f,bmp", bmp_data.temperature);
    //sprintf(all_data.bmp_pressure, "%d,bmp", bmp_data.pressure);
    //printf("%s,%s,%s,%s\n", all_data.dht_temp, all_data.dht_humidity, all_data.bmp_temp, all_data.bmp_pressure);

    connect_wifi();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    start_mqtt(all_data);
}

void app_main()
{
    boot_count += 1;
    ESP_LOGI(TAG, "Boot count: %d", boot_count);

    ESP_ERROR_CHECK(i2cdev_init());
    //connect_wifi();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    start();
    //start_mqtt();
}
