#ifndef BOND_MGMT_SERV_H__
#define BOND_MGMT_SERV_H__

void BMS_init(void);

void BMS_set_conn_handle(uint16_t conn_handle);

void BMS_on_ble_evt(ble_evt_t * p_ble_evt);

void delete_disconnected_bonds(void);

#endif // BOND_MGMT_SERV_H__
