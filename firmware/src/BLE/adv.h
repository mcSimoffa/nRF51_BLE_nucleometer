#ifndef ADV_H__
#define ADV_H__
#include "ble_advertising.h"

/*! ---------------------------------------------------------------------------
 \brief Function for initializing the Advertising functionality.
 */
void advertising_init(void);

/*! ---------------------------------------------------------------------------
* \brief Function for setting advertising mode before start.
 */
ret_code_t advertising_mode_set(ble_adv_mode_t advertising_mode, bool whitelist_en);

/*! ---------------------------------------------------------------------------
 * \brief Function for starting advertising.
 */
ret_code_t advertising_start(void);

void advertising_stop(void);


#endif // ADV_H__
