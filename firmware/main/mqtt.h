#ifndef _H_MQTT
#define _H_MQTT

#include "mqtt_client.h"

typedef struct message
{
    float temperature;
    float humidity;
    int pressure;
} message_t;

void mqtt_start(QueueHandle_t);

#endif
