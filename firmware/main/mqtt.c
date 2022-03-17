#include "mqtt.h"
#include "wifi.h"
#include "esp_sleep.h"
#include "esp_log.h"

#define MQTT_BROKER     CONFIG_MQTT_BROKER

#define TAG "MQTT"

static void enter_deep_sleep( void )
{
    ESP_LOGI(TAG, "Entering deep sleep");
    esp_deep_sleep(1e6 * 600);

}

static void send2broker(esp_mqtt_client_handle_t *client, QueueHandle_t queue)
{
    message_t message;

    if( xQueueReceive(queue, &(message), pdMS_TO_TICKS(1000)) )
    {
        ESP_LOGI(TAG, "New message from Queue");

        char payload[20];
        sprintf(payload, "%.1f,%.1f,%d", message.temperature, message.humidity, message.pressure);

        ESP_LOGI(TAG, "Sending payload to broker...");
        esp_mqtt_client_publish(*client, "home/metrics", payload, 0, 1, 0);
        ESP_LOGI(TAG, "Sent publish successful");
        ESP_LOGI(TAG, "Disconnecting client...");
        esp_mqtt_client_disconnect(*client);
    }
}

static void on_mqtt_connected(void *args, esp_event_base_t base, int32_t event_id, void *data)
{
    ESP_LOGI(TAG, "MQTT_CONNECTED");

    esp_mqtt_event_handle_t event = data;
    esp_mqtt_client_handle_t client = event->client;

    QueueHandle_t queue = (QueueHandle_t)args;
    send2broker(&client, queue);
}

static void on_mqtt_disconnected(void *args, esp_event_base_t base, int32_t event_id, void *data)
{
    ESP_LOGI(TAG, "MQTT_DISCONNECTED");

    disconnect_wifi();
    enter_deep_sleep();
}

void mqtt_start(QueueHandle_t queue)
{
    ESP_LOGI(TAG, "Starting MQTT client...");
    esp_mqtt_client_config_t cfg = {
        .uri = CONFIG_MQTT_BROKER,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(client, MQTT_EVENT_CONNECTED, on_mqtt_connected, (void *)queue);
    esp_mqtt_client_register_event(client, MQTT_EVENT_DISCONNECTED, on_mqtt_disconnected, NULL);
    esp_mqtt_client_start(client);
}
