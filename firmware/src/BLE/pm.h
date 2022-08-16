#ifndef PM_H__
#define PM_H__

#include "peer_manager.h"

void peer_manager_init(bool erase_bonds, ble_ctx_t *ctx);

void pm_evt_handler(pm_evt_t const * p_evt);

void pm_secure_initiate(uint16_t conn_handle);

#endif // PM_H__
