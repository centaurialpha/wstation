#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"
#include "sensor.h"

#define DHT_SENSOR_GPIO 4
#define SDA_GPIO 14
#define SCL_GPIO 13

#define TAG "SENSOR"

typedef struct message
{
    float temperature;
    float humidity;
    int pressure;
} message_t;

void collect_dht_data(float *temperature, float *humidity)
{
    esp_err_t err = dht_read_float_data(DHT_TYPE_DHT11, DHT_SENSOR_GPIO, humidity, temperature);
    if( err != ESP_OK )
    {
        ESP_LOGE(TAG, "Error reading DHT11 sensor.");
    }
}

void collect_bmp_data(float *temperature, uint32_t *pressure)
{
    ESP_ERROR_CHECK(i2cdev_init());

    bmp180_dev_t dev;
    memset(&dev, 0, sizeof(bmp180_dev_t));
    ESP_ERROR_CHECK(bmp180_init_desc(&dev, 0, SDA_GPIO, SCL_GPIO));
    ESP_ERROR_CHECK(bmp180_init(&dev));

    esp_err_t err = bmp180_measure(&dev, temperature, pressure, BMP180_MODE_ULTRA_LOW_POWER);
    if( err != ESP_OK )
    {
        ESP_LOGE(TAG, "Error reading BMP180 sensor.");
    }
}

void collect_data( void *pvParameters )
{
    QueueHandle_t queue = (QueueHandle_t) pvParameters;

    float temperature;
    float humidity;
    uint32_t pressure;

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    collect_dht_data(&temperature, &humidity);
    printf("\nDHT=%f, %f\n\n", temperature, humidity);
    collect_bmp_data(&temperature, &pressure);
    printf("\nBMP=%f, %d\n\n", temperature, pressure);

    message_t message = {
        .temperature = temperature,
        .humidity = humidity,
        .pressure = pressure,
    };

    xQueueSend(queue, (void *)&message, pdMS_TO_TICKS(200));

    vTaskDelete(NULL);
}
