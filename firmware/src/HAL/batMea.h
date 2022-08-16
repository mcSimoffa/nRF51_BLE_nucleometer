#ifndef BATMEA_H
#define BATMEA_H

#define INVALID_BATT_VOLTAGE    0xFFFF

typedef void (*bat_cb_t)(uint16_t mv, void *bat_ctx);

bool batMea_Start(bat_cb_t bat_cb, void *bat_ctx);

void batMea_Process(void);

#endif //BATMEA_H