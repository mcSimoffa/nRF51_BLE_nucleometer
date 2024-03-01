#ifndef BLE_MAIN_H__
#define BLE_MAIN_H__

typedef struct
{
  uint16_t  conn_handle;
  uint16_t  auth_rd_conn_handle;
} ble_ctx_t;

#define INPUT_OUTPUT_SERV         0xFDF0
#define IOS_SYSTIME_CHAR          0xFDF1
#define IOS_PULSE_CHAR            0xFDF2
#define IOS_INSTANT_VALUE_CHAR    0xFDF3
#define IOS_EVQ_CHAR              0xFDF4
#define IOS_EVQ_STATUS_CHAR       0xFDF5
#define IOS_TEMPERATURE_CHAR      0xFDF6
#define IOS_BATTERY_CHAR          0xFDF7
#define IOS_HW_PARAM_CHAR         0xFDF8

void BLE_Init(bool erase_bonds);

void BLE_Process(void);

void ble_ios_pulse_transfer(uint32_t pulse);

#endif // BLE_MAIN_H__
