#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "wifi.h"
#include "mqtt.h"
#include "sensor.h"

/**
 * @brief main function of the application.
 *
 * 1. Collect data from sensors.
 * 2. Start WiFi.
 * 3. Send collected data to MQTT broker.
 * 4. Enter in deep sleep mode.
 *
 */
void app_main()
{
    QueueHandle_t queue = xQueueCreate(2, sizeof(message_t));

    xTaskCreate(
            collect_data,
            "collect_data",
            configMINIMAL_STACK_SIZE * 4,
            (void *)queue, 5, NULL);

    connect_wifi();

    mqtt_start(queue);
}
