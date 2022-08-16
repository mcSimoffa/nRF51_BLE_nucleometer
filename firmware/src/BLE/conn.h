#ifndef CONN_H__
#define CONN_H__

#include "ble_main.h"

void gap_params_init(ble_ctx_t *ctx);

void static_passkey_def(void);

void conn_params_init(void);

void BLE_conn_handle_set(uint16_t hnd);

uint16_t BLE_conn_handle_get(void);

#endif // CONN_H__
