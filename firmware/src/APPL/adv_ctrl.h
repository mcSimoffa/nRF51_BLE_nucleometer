#ifndef ADV_CTRL_H
#define ADV_CTRL_H

#include "ble_main.h"
/*! ---------------------------------------------------------------------------
 \brief Function for initializing button double click processing.
 */
void adv_ctrl_Init(ble_ctx_t *ctx);

void adv_ctrl_Process(void);

void ble_advertising_on_sys_evt(uint32_t sys_evt);

#endif	// ADV_CTRL_H