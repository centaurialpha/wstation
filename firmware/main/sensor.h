#ifndef _SENSOR_H
#define _SENSOR_H

#include <dht.h>
#include <bmp180.h>

void collect_data( void *pvParameters );
void collect_dht_data( float *temperature, float *humidity );
void collect_bmp_data( float *temperature, uint32_t *pressure );

#endif
