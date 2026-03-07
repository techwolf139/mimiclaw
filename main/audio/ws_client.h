#ifndef WS_CLIENT_H
#define WS_CLIENT_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

typedef void (*ws_client_text_callback_t)(const char *text);
typedef void (*ws_client_connect_callback_t)(bool connected);
typedef void (*ws_client_handshake_callback_t)(bool done);

esp_err_t ws_client_init(const char *host, int port);
esp_err_t ws_client_connect(void);
esp_err_t ws_client_disconnect(void);
esp_err_t ws_client_send_audio(const uint8_t *data, size_t len);
esp_err_t ws_client_register_text_callback(ws_client_text_callback_t callback);
esp_err_t ws_client_register_connect_callback(ws_client_connect_callback_t callback);
esp_err_t ws_client_register_handshake_callback(ws_client_handshake_callback_t callback);
bool ws_client_is_connected(void);
bool ws_client_is_handshake_done(void);

#endif
