#ifndef ADV_H__
#define ADV_H__
#include "ble_advertising.h"

void ble_advertising_on_sys_evt(uint32_t sys_evt);

void advertising_init(void);

ret_code_t advertising_mode_set(ble_adv_mode_t advertising_mode, bool whitelist_en);

void advertising_start(void);

void advertising_stop(void);


#endif // ADV_H__
