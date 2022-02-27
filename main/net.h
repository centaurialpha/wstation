#ifndef _H_NET
#define _H_NET

#include "esp_err.h"

void connect_wifi( void );
void disconnect_wifi( void );
esp_err_t send_post(char *);

#endif
