#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sleep.h"

#include "sensor.h"
#include "net.h"

#define TAG "WSTATION"

RTC_DATA_ATTR static int boot_count = 0;

void publish_data(void *args)
{

    dht_data_t dht_data = {0};
    bmp_data_t bmp_data = {0};
    char data_buf[80];
    char data_buf_2[80];

    ESP_ERROR_CHECK(bmp_init());

    // DHT Sensor
    if( get_dht_sensor_data(&dht_data) != ESP_OK )
    {
        printf("Error reading sensor..\n");
    }
    printf("%f C - %f F - %f H STATE %d\n", dht_data.temperature_C, dht_data.temperature_F, dht_data.humidity, dht_data.state);
    sprintf(data_buf,
            "{\"tempf\":%f,\"tempc\":%f,\"humidity\":%f,\"s_state\":%d}",
            dht_data.temperature_F,
            dht_data.temperature_C,
            dht_data.humidity,
            dht_data.state);
    printf("%s\n", data_buf);
    ESP_ERROR_CHECK(send_post(data_buf));

    // BMP Sensor
    if( get_bmp_sensor_data(&bmp_data) != ESP_OK)
    {
        printf("Error reading sensor...\n");
    }
    printf("%f C - %f F - %dHPa State %d\n", bmp_data.temperature_C, bmp_data.temperature_F, bmp_data.pressure, bmp_data.state);
    sprintf(data_buf_2,
            "{\"tempf\":%f,\"tempc\":%f,\"pressure\":%d,\"s_state\":%d}",
            bmp_data.temperature_F,
            bmp_data.temperature_C,
            bmp_data.pressure,
            bmp_data.state);
    ESP_ERROR_CHECK(send_post(data_buf_2));

    disconnect_wifi();
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    const int deep_sleep_secs = 1800;
    ESP_LOGI(TAG, "Entering deep sleep for %d seconds", deep_sleep_secs);
    esp_deep_sleep(1e6 * deep_sleep_secs);
}

void app_main()
{
    boot_count += 1;
    ESP_LOGI(TAG, "Boot count: %d", boot_count);

    ESP_ERROR_CHECK(i2cdev_init());
    connect_wifi();
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    xTaskCreate(publish_data, "Publisher", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
}
