#ifndef BLE_MAIN_H__
#define BLE_MAIN_H__

#define INPUT_OUTPUT_SERV 0xFDF0
#define IOS_SYSTIME_CHAR  0xFDF1
#define IOS_PULSE_CHAR    0xFDF2
#define IOS_DEVSTAT_CHAR  0xFDF3
#define IOS_HW_PARAM_CHAR 0xFDF4

void BLE_Init(void);

void BLE_Process(void);

#endif // BLE_MAIN_H__
