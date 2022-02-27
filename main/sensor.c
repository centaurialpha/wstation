#include <string.h>
#include "sensor.h"

#define DHT_SENSOR_GPIO 4
#define SDA_GPIO 14
#define SCL_GPIO 13

static bmp180_dev_t dev;

static float c2f(float value)
{
    return (value * 1.8) + 32;
}

esp_err_t get_dht_sensor_data(dht_data_t *data)
{
    esp_err_t err = dht_read_float_data(DHT_TYPE_DHT11, DHT_SENSOR_GPIO, &data->humidity, &data->temperature_C);
    if( err == ESP_OK ) {
        data->temperature_F = c2f(data->temperature_C);
        data->state = 1;
    } else {
        data->temperature_C = .0;
        data->temperature_F = .0;
        data->humidity = 0;
        data->state = 0;
    }
    return err;
}

esp_err_t bmp_init()
{
   memset(&dev, 0, sizeof(bmp180_dev_t));

   ESP_ERROR_CHECK(bmp180_init_desc(&dev, 0, SDA_GPIO, SCL_GPIO));

   return bmp180_init(&dev);
}

esp_err_t get_bmp_sensor_data(bmp_data_t *data)
{
    esp_err_t err = bmp180_measure(&dev, &data->temperature_C, &data->pressure, BMP180_MODE_ULTRA_LOW_POWER);
    if( err == ESP_OK ) {
        data->temperature_F = c2f(data->temperature_C);
        data->state = 1;
    } else {
        data->temperature_C = .0;
        data->temperature_F = .0;
        data->pressure = 0;
        data->state = 0;
    }
    return err;
}
