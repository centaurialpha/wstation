#ifndef _SENSOR_H
#define _SENSOR_H

#include <dht.h>
#include <bmp180.h>

typedef struct dht_data_sensor_t
{
    float humidity;
    float temperature_C;
    float temperature_F;
    uint8_t state;

} dht_data_t;

typedef struct bmp_data_sensor_t
{
    float temperature_C;
    float temperature_F;
    uint32_t pressure;
    uint8_t state;

} bmp_data_t;

esp_err_t bmp_init();
esp_err_t get_dht_sensor_data(dht_data_t*);
esp_err_t get_bmp_sensor_data(bmp_data_t*);

#endif
