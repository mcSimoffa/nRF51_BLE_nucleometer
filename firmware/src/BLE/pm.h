#ifndef PM_H__
#define PM_H__

#include "peer_manager.h"

void peer_manager_init(bool erase_bonds);

void pm_evt_handler(pm_evt_t const * p_evt);

#endif // PM_H__
