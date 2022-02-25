#include <string.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#include "esp_log.h"
#include "esp_netif.h"

#include "sensor.h"
#include "net.h"

#define PUSH_DATA_DHT_PERIOD_S CONFIG_DHT_INTERVAL
#define PUSH_DATA_BMP_PERIOD_S CONFIG_BMP_INTERVAL

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

void publish_data_bmp180(void *args)
{
    bmp_data_t data = {0};
    char data_buf[80];

    ESP_ERROR_CHECK(bmp_init());

    while (1)
    {
        if( get_bmp_sensor_data(&data) != ESP_OK)
        {
            printf("Error reading sensor...\n");
        }
        printf("%f C - %f F - %dHPa State %d\n", data.temperature_C, data.temperature_F, data.pressure, data.state);
        sprintf(data_buf,
                "{\"tempf\":%f,\"tempc\":%f,\"pressure\":%d,\"s_state\":%d}",
                data.temperature_F,
                data.temperature_C,
                data.pressure,
                data.state);
        ESP_ERROR_CHECK(send_post(data_buf));

        vTaskDelay((PUSH_DATA_BMP_PERIOD_S * 1000) / portTICK_PERIOD_MS);
    }
}

void publish_data(void *args)
{
    dht_data_t data = {0};

    char data_buf[80];


    while( 1 )
    {
        if( get_dht_sensor_data(&data) != ESP_OK )
        {
            printf("Error reading sensor..\n");
        }
        printf("%f C - %f F - %f H STATE %d\n", data.temperature_C, data.temperature_F, data.humidity, data.state);
        sprintf(data_buf,
                "{\"tempf\":%f,\"tempc\":%f,\"humidity\":%f,\"s_state\":%d}",
                data.temperature_F,
                data.temperature_C,
                data.humidity,
                data.state);
        ESP_ERROR_CHECK(send_post(data_buf));

        vTaskDelay((PUSH_DATA_DHT_PERIOD_S * 1000) / portTICK_PERIOD_MS);
    }

}

void app_main()
{
    ESP_ERROR_CHECK(i2cdev_init());
    wifi_start();

    xTaskCreate(blink, "BLINK", 2048, NULL, 1, NULL);
    xTaskCreate(publish_data, "PUBLISHER", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
    xTaskCreate(publish_data_bmp180, "PUBLISHER 2", 4096, NULL, 6, NULL);
}
