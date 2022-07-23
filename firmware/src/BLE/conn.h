#ifndef CONN_H__
#define CONN_H__

void gap_params_init(void);

void static_passkey_def(void) ;

void conn_params_init(void);

void BLE_conn_handle_set(uint16_t hnd);

uint16_t BLE_conn_handle_get(void);

#endif // CONN_H__
